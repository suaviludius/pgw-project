#include "logger.h"
#include "Config.h"
#include "FileCdrWriter.h"
#include "DatabaseCdrWriter.h"
#include "CdrWriterFactory.h"
#include "SessionManager.h"
#include "SocketFactory.h"
#include "UdpServer.h"
#include "HttpServer.h"

#include <poll.h>
#include <atomic>
#include <chrono>
#include <iostream> // cerr
#include <memory>

// Миллисекундный таймаут для poll()
constexpr int POLL_TIMEOUT_MS {100};

// Cекундный таймер очищения просроченных сессий
constexpr int SESSION_TIMEOUT_S {10};

int main(int argc, char* argv[]){
    try{
        std::string configPath = "configs/pgw_server.json";
        // Если передан аргумент - используем его
        if (argc > 1) {
            configPath = argv[1];
        }

        // Читаем конфигурацию из JSON файла
        pgw::server::Config config(configPath);

        // Инициализируем логгер (консоль) параметрами из конфиг файла
        pgw::logger::init(config.getLogLevel());

        // Инициализируем менеджер базы данных (единый для CDR и логов)
        auto dbManager = std::make_shared<pgw::DatabaseManager>("pgw.db");

        // Инициализируем CDR writer
        std::unique_ptr<pgw::ICdrWriter> cdrWriter;

        if (!dbManager->initialize()) {
            LOG_ERROR("Failed to initialize database, continuing with limited functionality");
            // Инициализируем с помощью файлов
            cdrWriter = pgw::CdrWriterFactory::createFile(config.getCdrFile());
            pgw::logger::addFileSink(std::string(config.getLogFile()));
        } else {
            LOG_INFO("Initialize database sucsessful");
            // Инициализируем с помощью БД
            cdrWriter = pgw::CdrWriterFactory::createDatabase(dbManager);
            pgw::logger::addDatabaseSink(dbManager);
        }

        LOG_INFO("Application started");

        // Инициализируем менеджер сессий
        pgw::SessionManager sessionManager(
            *cdrWriter,
            config.getBlacklist(),
            config.getSessionTimeoutSec(),
            config.getGracefulShutdownRate()
        );

        // Инициализируем сервер на прием сообщений от клиента
        pgw::UdpServer udpServer(
            sessionManager,
            config.getUdpIp(),
            config.getUdpPort(),
            pgw::SocketFactory::createUdp()
        );

        // Флаг запроса на завершение работы (разделяемый между потоками)
        std::atomic<bool> shutdownRequest{false};

        // Инициализируем HTTP сервер для мониторинга и управления
        pgw::HttpServer httpServer(
            sessionManager,
            config.getHttpPort(),
            shutdownRequest
        );

        // Запускаем UDP сервер (работает в текущем потоке)
        udpServer.start();

        // Запускаем HTTP сервер в отдельном потоке
        httpServer.start();
        // Примеры HTTP запросов:
        // Проверка статуса абонента:
        // curl "http://localhost:8080/check_subscriber?imsi=250990123456789"
        // Остановка сервера:
        // curl -X POST http://localhost:8080/stop

        // Добавляем UDP сокет для отслеживания входящих пакетов
        pollfd udpFd{};
        udpFd.fd = udpServer.getFd();
        udpFd.events = POLLIN;  // Ждём данные для чтения

        // Таймер для очищения сессий
        auto lastCleanup = std::chrono::steady_clock::now();

        // Выполняем работу сервера пока не придёт запрос на shutdown
        while(!shutdownRequest.load(std::memory_order_acquire)){
            // Ждём события на UDP сокете с таймаутом 100ms
            int ready = poll(&udpFd, 1, POLL_TIMEOUT_MS);

            if(ready == -1){
                if (errno == EINTR) continue;  // Прервано сигналом
                LOG_ERROR("Poll error: " + std::string(strerror(errno)));
                break;
            }

            // Обработка UDP пакетов, если есть данные
            if (ready > 0 && (udpFd.revents & POLLIN)) {
                // Обрабатываем все доступные пакеты
                udpServer.handler();
            }

            // Периодические задачи (выполняются по таймауту)
            auto now = std::chrono::steady_clock::now();

            // Очистка просроченных сессий (каждые SESSION_TIMEOUT_S секунд)
            if (now - lastCleanup > std::chrono::seconds(SESSION_TIMEOUT_S)) {
               sessionManager.cleanTimeoutSessions();
               lastCleanup = now;
            }
        }

        // Удаляем сессии в менеджере (graceful shutdown)
        sessionManager.gracefulShutdown();

        // Останавливаем работу серверов
        httpServer.stop();
        udpServer.stop();

        LOG_INFO("Application stopped gracefully");
    }
    catch(const std::exception& e){
        if(pgw::logger::isInit()){
            // Критическая ошибка при выполнении
            LOG_CRITICAL("Fatal error: {}", e.what());
            pgw::logger::shutdown();
        }
        else {
            // Логгер не инициализирован - вывод в stderr
            std::cerr << "Fatal error: " << e.what() << '\n';
        }

        return 1;
    }

    pgw::logger::shutdown();
    return 0;
}