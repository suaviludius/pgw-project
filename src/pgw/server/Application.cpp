#include "Application.h"
#include "Config.h"
#include "logger.h"
#include "CdrWriterFactory.h"
#include "SessionManager.h"
#include "SocketFactory.h"
#include "UdpServer.h"
#include "HttpServer.h"
#include "DatabaseManager.h"

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

Application::Application(const std::string& configPath)
    : m_config{std::make_unique<Config>(configPath)} {
}

Application::~Application() {
    if (m_udpServer && m_udpServer->isRunning()) {
        m_udpServer->stop();
    }
    if (m_httpServer && m_httpServer->isRunning()) {
        m_httpServer->stop();
    }
    if (pgw::logger::isInit()) {
        pgw::logger::shutdown();
    }
}

int Application::run() {
    try {
        // Проверка валидности конфигурации
        if (!m_config->isValid()) {
            std::cerr << "Config error: " << m_config->getError() << '\n';
            return 1;
        }

        // Последовательная инициализация компонентов
        if (!initializeLogger()) {
            std::cerr << "Failed to initialize logger\n";
            return 1;
        }

        if (!initializeDatabase()) {
            LOG_ERROR("Failed to initialize database");
            return 1;
        }

        if (!initializeCdrWriter()) {
            LOG_ERROR("Failed to initialize CDR writer");
            return 1;
        }

        if (!initializeSessionManager()) {
            LOG_ERROR("Failed to initialize session manager");
            return 1;
        }

        if (!initializeServers()) {
            LOG_ERROR("Failed to initialize servers");
            return 1;
        }

        LOG_INFO("Application started successfully");

        // Запуск основного цикла обработки событий
        runEventLoop();

        // Graceful shutdown
        shutdown();

        LOG_INFO("Application stopped gracefully");
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

bool Application::initializeLogger() {
    pgw::logger::init(m_config->getLogLevel());
    return pgw::logger::isInit();
}

bool Application::initializeDatabase() {
    m_dbManager = std::make_shared<DatabaseManager>(std::string(m_config->getDatabaseFile()));

    if (!m_dbManager->initialize()) {
        LOG_ERROR("Failed to initialize database, continuing with limited functionality");
        m_dbManager.reset();
        return true; // Продолжаем работу без БД
    }

    LOG_INFO("Database initialized successfully");
    return true;
}

bool Application::initializeCdrWriter() {
    if (m_dbManager) {
        // Используем базу данных для CDR
        m_cdrWriter = CdrWriterFactory::createDatabase(m_dbManager);
        // Добавляем Database sink для логирования
        pgw::logger::addDatabaseSink(m_dbManager);
        LOG_INFO("CDR writer initialized with database backend");
    } else {
        // Используем файл для CDR (fallback)
        m_cdrWriter = CdrWriterFactory::createFile(m_config->getCdrFile());
        // Добавляем файловый sink для логирования
        pgw::logger::addFileSink(std::string(m_config->getLogFile()));
        LOG_INFO("CDR writer initialized with file backend");
    }

    return true;
}

bool Application::initializeSessionManager() {
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

bool Application::initializeServers() {
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

    // Запуск серверов
    m_udpServer->start();
    m_httpServer->start();

    LOG_INFO("Servers started: UDP on {}:{}, HTTP on port {}",
             m_config->getUdpIp(), m_config->getUdpPort(), m_config->getHttpPort());

    return true;
}

void Application::runEventLoop() {
    // Настройка poll для отслеживания UDP сокета
    pollfd udpFd{};
    udpFd.fd = m_udpServer->getFd();
    udpFd.events = POLLIN;

    // Таймер для периодической очистки просроченных сессий
    auto lastCleanup = std::chrono::steady_clock::now();

    // Основной цикл обработки событий
    while (!m_shutdownRequest.load(std::memory_order_acquire)) {
        // Ожидание событий на UDP сокете с таймаутом
        int ready = poll(&udpFd, 1, POLL_TIMEOUT_MS);

        if (ready == -1) {
            if (errno == EINTR) {
                continue;  // Прервано сигналом, продолжаем
            }
            LOG_ERROR("Poll error: {}", strerror(errno));
            break;
        }

        // Обработка UDP пакетов, если есть данные
        if (ready > 0 && (udpFd.revents & POLLIN)) {
            m_udpServer->handler();
        }

        // Периодические задачи
        auto now = std::chrono::steady_clock::now();

        // Очистка просроченных сессий
        if (now - lastCleanup > std::chrono::seconds(SESSION_CLEANUP_INTERVAL_S)) {
            m_sessionManager->cleanTimeoutSessions();
            lastCleanup = now;
        }
    }
}

void Application::shutdown() {
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

    LOG_INFO("Graceful shutdown completed");
}

} // namespace server
} // namespace pgw
