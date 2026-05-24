#include "Server.h"

#include "pgw/common/logger.h"
#include "pgw/common/SocketFactory.h"
#include "Config.h"
#include "DatabaseManager.h"
#include "CdrWriterFactory.h"
#include "SessionManager.h"
#include "TcpHandler.h"
#include "TcpServer.h"
#include "UdpServer.h"
#include "HttpServer.h"

#include <poll.h>
#include <chrono>
#include <iostream>
#include <cerrno>
#include <cstring>

// Таймаут для poll() в миллисекундах
constexpr int POLL_TIMEOUT_MS {100};

// Интервал очистки просроченных сессий в секундах
constexpr int SESSION_CLEANUP_INTERVAL_S {10};

namespace pgw {
namespace server {

// Определение структуры Impl – все поля и приватные методы класса
struct Server::Impl {
    // Конфигурация приложения (создается один раз в конструкторе)
    std::unique_ptr<Config> m_config;

    // Флаг завершения работы
    std::atomic<bool> m_shutdownRequest{false};

    // Компоненты приложения
    std::shared_ptr<DatabaseManager> m_dbManager;
    std::shared_ptr<ICdrWriter> m_cdrWriter;
    std::unique_ptr<SessionManager> m_sessionManager;
    std::unique_ptr<TcpHandler> m_tcpHandler;
    std::unique_ptr<TcpServer> m_tcpServer;
    std::unique_ptr<UdpServer> m_udpServer;
    std::unique_ptr<HttpServer> m_httpServer;

    // Инициализация компонентов
    bool initializeLogger();
    bool initializeDatabase();
    bool initializeCdrWriter();
    bool initializeSessionManager();
    bool initializeServers();

    // Основной цикл обработки событий
    void runEventLoop();

    // Graceful shutdown
    void shutdown();

    // Конструктор принимает configPath
    explicit Impl(const std::string& configPath)
        : m_config(std::make_unique<Config>(configPath)) {}
};

Server::Server(const std::string& configPath)
    : pImpl{std::make_unique<Impl>(configPath)} {
}

// Все серверы сами остановятся
Server::~Server() = default;

void Server::stop(){
    pImpl->m_shutdownRequest.store(true);
}

int Server::run() {
    try {
        // Проверка валидности конфигурации
        if (!pImpl->m_config->isValid()) {
            std::cerr << "Config error: " <<  pImpl->m_config->getError() << '\n';
            return 1;
        }

        // Последовательная инициализация компонентов
        if (!pImpl->initializeLogger()) {
            std::cerr << "Failed to initialize logger\n";
            return 1;
        }

        if (!pImpl->initializeDatabase()) {
            LOG_WARN("Failed to initialize database");
        }

        if (!pImpl->initializeCdrWriter()) {
            LOG_ERROR("Failed to initialize CDR writer");
            return 1;
        }

        if (!pImpl->initializeSessionManager()) {
            LOG_ERROR("Failed to initialize session manager");
            return 1;
        }

        if (!pImpl->initializeServers()) {
            LOG_ERROR("Failed to initialize servers");
            return 1;
        }

        LOG_INFO("Server started successfully");

        // Запуск основного цикла обработки событий
        pImpl->runEventLoop();

        // Graceful shutdown
        pImpl->shutdown();

        LOG_INFO("Server stopped gracefully");
        return 0;
    }
    catch (const std::exception& e) {
        if (pgw::logger::isInit()) {
            LOG_CRITICAL("Fatal error: {}", e.what());
            pgw::logger::shutdown();
        } else {
            std::cerr << "Fatal error: " << e.what() << '\n';
        }
        return 1;
    }
}

bool Server::Impl::initializeLogger() {
    pgw::logger::init(m_config->getLogLevel());
    pgw::logger::addFileSink(std::string(m_config->getLogFile()));
    return pgw::logger::isInit();
}

bool Server::Impl::initializeDatabase() {
    std::string dbPath(m_config->getDatabaseFile());
    if (dbPath.empty()) return false;

    m_dbManager = std::make_shared<DatabaseManager>(dbPath);

    if (!m_dbManager->initialize()) {
        m_dbManager.reset();
        return false; // Продолжаем работу без БД
    }

    LOG_INFO("Database initialized successfully");
    return true;
}

bool Server::Impl::initializeCdrWriter() {
    if (m_dbManager) {
        // Используем базу данных для CDR
        m_cdrWriter = CdrWriterFactory::createDatabase(m_dbManager);
        LOG_INFO("CDR writer initialized with database backend");
        return true;
    }

    // Используем файл для CDR (fallback)
    m_cdrWriter = CdrWriterFactory::createFile(m_config->getCdrFile());
    LOG_INFO("CDR writer initialized with file backend");

    return true;
}

bool Server::Impl::initializeSessionManager() {
    if (!m_cdrWriter) {
        LOG_ERROR("CDR writer not initialized");
        return false;
    }

    m_sessionManager = std::make_unique<SessionManager>(
        *m_cdrWriter,
        m_config->getBlacklist(),
        m_config->getSessionTimeoutSec(),
        m_config->getGracefulShutdownRate()
    );

    LOG_INFO("Session manager initialized");
    return true;
}

bool Server::Impl::initializeServers() {
    if (!m_sessionManager) {
        LOG_ERROR("Session manager not initialized");
        return false;
    }

    // Инициализация UDP сервера
    m_udpServer = std::make_unique<UdpServer>(
        *m_sessionManager,
        m_config->getUdpIp(),
        m_config->getUdpPort(),
        SocketFactory::createUdp()
    );

    // Инициализация HTTP сервера
    m_httpServer = std::make_unique<HttpServer>(
        *m_sessionManager,
        m_config->getHttpPort(),
        m_shutdownRequest
    );

    // Инициализация обработчика TCP команд
    m_tcpHandler = std::make_unique<TcpHandler>(
        *m_sessionManager,
        m_cdrWriter,
        m_shutdownRequest
    );

    // Инициализация TCP сервера
    m_tcpServer = std::make_unique<TcpServer>(
        m_config->getTcpIp(),
        m_config->getTcpPort(),
        *m_tcpHandler
    );

    // Запуск серверов
    m_udpServer->start();
    m_httpServer->start();
    m_tcpServer->start();

    LOG_INFO("Servers started: UDP on {}:{}, HTTP on port {}, TCP on port {}",
             m_config->getUdpIp(), m_config->getUdpPort(), m_config->getHttpPort(), m_config->getTcpPort());

    return true;
}

void Server::Impl::runEventLoop() {
    // Настройка poll для отслеживания TCP и UDP сокетов
    constexpr int MAX_FDS = 10;
    std::vector<struct pollfd> fds;

    // Таймер для периодической очистки просроченных сессий
    auto lastCleanup = std::chrono::steady_clock::now();

    // Основной цикл обработки событий
    while (!m_shutdownRequest.load(std::memory_order_acquire)) {

        // Индексы для фиксации позиций серверов
        int udpIdx = -1;
        int tcpIdx = -1;

        // UDP сокет
        if (m_udpServer && m_udpServer->isRunning()){
            fds.push_back({
                .fd = m_udpServer->getFd(),
                .events = POLLIN,
                .revents = 0
            });
            udpIdx = fds.size() - 1;
        }

        // TCP сокет
        if (m_tcpServer && m_tcpServer->isRunning()) {
            fds.push_back({
                .fd = m_tcpServer->getFd(),
                .events = POLLIN,
                .revents = 0
            });
            tcpIdx = fds.size() - 1;
        }

        // Добавляем клиентские сокеты динамически
        auto clientsFds = m_tcpServer->getClientsFds();
        for (int clientFd : clientsFds) {
            fds.push_back({
                .fd = clientFd,
                .events = POLLIN,
                .revents = 0
            });
        }

        // Ожидание событий на всех сокетах с таймаутом
        int ready = poll(fds.data(), fds.size(), POLL_TIMEOUT_MS);

        if (ready == -1) {
            if (errno == EINTR) continue; // Прервано сигналом, продолжаем
            LOG_ERROR("Poll error: {}", strerror(errno));
            break;
        }

        // Обработка UDP пакетов, если есть данные
        if (udpIdx != -1 && (fds[udpIdx].revents & POLLIN)) {
            m_udpServer->processEvent();
        }

        // Обработка подключений TCP сервера
        if (tcpIdx != -1 && (fds[tcpIdx].revents & POLLIN)) {
            // Новый клиент пытается подключиться
            m_tcpServer->acceptNewClient();
        }

        // Так мы узнаем индекс первого клиента
        size_t firstClientIdx = (udpIdx != -1 ? 1 : 0) + (tcpIdx != -1 ? 1 : 0);

        // Обработка команд TCP сервера
        for (size_t i = firstClientIdx; i < fds.size(); ++i) {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                // Передаем конкретный fd клиента, в котором произошло событие.
                m_tcpServer->handleClientData(fds[i].fd);
            }
        }

         if (ready == 0) {
            // Таймаут, переходим к периодическим задачам
            auto now = std::chrono::steady_clock::now();

            // Очистка просроченных сессий
            if (now - lastCleanup > std::chrono::seconds(SESSION_CLEANUP_INTERVAL_S)) {
                m_sessionManager->cleanTimeoutSessions();
                lastCleanup = now;
            }
        }
    }
}

void Server::Impl::shutdown() {
    LOG_INFO("Starting graceful shutdown...");

    // Завершение всех сессий с контролируемой скоростью
    if (m_sessionManager) {
        m_sessionManager->gracefulShutdown();
    }

    // Остановка серверов
    if (m_httpServer) {
        m_httpServer->stop();
    }
    if (m_udpServer) {
        m_udpServer->stop();
    }
    if (m_tcpServer) {
        m_tcpServer->stop();
    }

    LOG_INFO("Graceful shutdown completed");
}

ISessionManager& Server::getSessionManager() {
    return *pImpl->m_sessionManager;
}

std::shared_ptr<ICdrWriter> Server::getCdrWriter() {
    return pImpl->m_cdrWriter;
}

} // namespace server
} // namespace pgw
