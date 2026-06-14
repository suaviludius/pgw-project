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

#include <sys/epoll.h>
#include <sys/timerfd.h>
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

        LOG_INFO("Main server started successfully");

        // Запуск основного цикла обработки событий
        pImpl->runEventLoop();

        LOG_INFO("Main server stopped gracefully");
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

    // Состояния, в которых может находиться цикл
    enum class State {
        RUNNING,    // Нормальная работа
        DRAINING,   // Завершаем сессии
        STOPPING,   // Останавливаем серверы
        STOPPED     // Полностью остановлен
    };
    State state = State::RUNNING;

    // Создаём epoll-дескриптор
    int epollFd = epoll_create1(0);

    if (epollFd == -1) {
        LOG_CRITICAL("Epoll create failed: {}", strerror(errno));
        return;
    }

    // Вспомогательная лямбда для добавления fd в epoll
    auto addToEpoll = [epollFd](int fd, uint32_t events) {
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;
        if (epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&ev) == -1) {
            LOG_ERROR("epoll_ctl add failed for fd {} : {}", fd, strerror(errno));
        }
    };

    // Таймер для shutdown
    int timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    //addToEpoll(timerFd, EPOLLIN);

    // Серверный UDP сокет
    if (m_udpServer && m_udpServer->isRunning()){
        addToEpoll(m_udpServer->getFd(), EPOLLIN);
    }

    // Серверный TCP сокет
    if (m_tcpServer && m_tcpServer->isRunning()) {
        addToEpoll(m_tcpServer->getFd(), EPOLLIN);
    }

    // Настройка epoll для отслеживания TCP и UDP сокетов
    constexpr int MAX_FDS = 10;
    struct epoll_event events[MAX_FDS];

    // Таймер для периодической очистки просроченных сессий
    auto lastCleanup = std::chrono::steady_clock::now();

    // Таймер для выполнения shutdown
    auto shutdownInterval = 1000 /(m_sessionManager->getShutdownRate());
    struct itimerspec timerSpec;
    timerSpec.it_value.tv_sec = 0;
    timerSpec.it_value.tv_nsec = shutdownInterval * 1000000;  // 10ms
    timerSpec.it_interval.tv_sec = 0;
    timerSpec.it_interval.tv_nsec = shutdownInterval * 1000000;  // Периодический

    // Основной цикл обработки событий
    while (state != State::STOPPED) {

        // Настройки при shutdown request
        if(state == State::RUNNING && m_shutdownRequest.load(std::memory_order_acquire)){
            LOG_INFO("Starting graceful shutdown");
            m_sessionManager->startDraining();
            if (m_tcpServer) m_tcpServer->stopAccepting();

            // Запускаем таймер
            timerfd_settime(timerFd, 0, &timerSpec, nullptr);
            // Теперь в epoll() будет обрабатываться shutdown таймер
            addToEpoll(timerFd, EPOLLIN);
            state = State::DRAINING;
        }

        if(state == State::STOPPING){
            LOG_DEBUG("Servers stopping");
            if (m_udpServer) m_udpServer->stop();
            if (m_tcpServer) m_tcpServer->stop();
            if (m_httpServer) m_httpServer->stop();
            LOG_DEBUG("Servers stopped");
            state = State::STOPPED;
        }

        if(state != State::STOPPED){
            // Ожидание событий на всех сокетах с таймаутом, вернет количество сигналов
            int nfds = epoll_wait(epollFd, events, MAX_FDS, POLL_TIMEOUT_MS);

            if (nfds == -1) {
                if (errno == EINTR) continue; // Прервано сигналом, продолжаем
                LOG_ERROR("Epoll error: {}", strerror(errno));
                break;
            }

            // Получаем количество сокетов, что хотят разговаривать
            for (int i = 0; i < nfds; ++i) {
                int fd = events[i].data.fd;
                uint32_t ev = events[i].events;

                // Определяем, что это за сокет (по fd)
                if (state == State::DRAINING && m_sessionManager && fd == timerFd) {
                    if (ev & EPOLLIN) {
                        // Считываем сколько раз сработал shutdown таймер
                        uint64_t expirations;
                        ssize_t s = read(timerFd, &expirations, sizeof(uint64_t));

                        // Удаляем по количеству сессий, накопившихся в таймере
                        for (uint64_t i = 0; i < expirations; ++i) {
                            if(!(m_sessionManager->shutdownSession())) {
                                LOG_INFO("All sessions terminated");
                                state = State::STOPPING;
                                break;
                            }
                        }
                    }
                }
                // Серверный UDP соккет ???
                else if (m_udpServer && fd == m_udpServer->getFd()) {
                    if (ev & EPOLLIN) m_udpServer->processEvent();
                }
                // Серверный TCP соккет ???
                else if (m_tcpServer && fd == m_tcpServer->getFd()) {
                    if (ev & EPOLLIN) {
                        int clientFd = m_tcpServer->acceptNewClient();
                        if (clientFd != -1) {
                            // Добавить клиентский сокет в epoll
                            addToEpoll(clientFd, EPOLLIN | EPOLLRDHUP);
                        }
                    }
                }
                // Клиентский TCP соккет !!!
                else {
                    if (ev & (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                        if (!m_tcpServer->handleClientData(fd)) {
                            // Если соединение закрыто, удаляем из epoll
                            epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                        }
                    }
                }
            }

            // Таймаут, переходим к периодическим задачам
            auto now = std::chrono::steady_clock::now();

            // Очистка просроченных сессий
            if (now - lastCleanup > std::chrono::seconds(SESSION_CLEANUP_INTERVAL_S / 3)) {
                m_sessionManager->cleanTimeoutSessions();
                lastCleanup = now;
            }
        }
    }

    close(epollFd);
}


ISessionManager& Server::getSessionManager() {
    return *pImpl->m_sessionManager;
}

std::shared_ptr<ICdrWriter> Server::getCdrWriter() {
    return pImpl->m_cdrWriter;
}

} // namespace server
} // namespace pgw
