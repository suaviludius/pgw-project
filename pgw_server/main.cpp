#include "logger.h"
#include "Config.h"
#include "CdrWriter.h"
#include "SessionManager.h"
#include "UdpSocket.h"
#include "UdpServer.h"
#include "HttpServer.h"

#include <poll.h>
#include <atomic>
#include <chrono>
#include <iostream>

// Миллисекундный таймаут для poll()
constexpr int POLL_TIMEOUT_MS {100};

// Секундный таймер очищения просроченных сессий
constexpr int SESSION_TIMEOUT_S {10};

int main(int argc, char* argv[]){
    try{
        std::string configPath = "configs/pgw_server.json";
        if (argc > 1) {
            configPath = argv[1];
        }

        // Читаем конфигурацию из JSON файла
        pgw::server::Config config(configPath);

        // Инициализируем логгер
        pgw::logger::init(
            config.getLogFile(),
            config.getLogLevel()
        );

        // Инициализируем журнал создания сессий
        pgw::CdrWriter cdrWriter(
            config.getCdrFile()
        );

        // Инициализируем менеджер сессий
        pgw::SessionManager sessionManager(
            cdrWriter,
            config.getBlacklist(),
            config.getSessionTimeoutSec(),
            config.getGracefulShutdownRate()
        );

        // Инициализируем UDP сервер
        pgw::UdpServer udpServer(
            sessionManager,
            config.getUdpIp(),
            config.getUdpPort()
        );

        // Флаг запроса на завершение работы
        std::atomic<bool> shutdownRequest{false};

        // Инициализируем HTTP сервер
        pgw::HttpServer httpServer(
            sessionManager,
            config.getHttpPort(),
            shutdownRequest
        );

        // Запускаем UDP сервер
        udpServer.start();

        // Запускаем HTTP сервер в отдельном потоке
        httpServer.start();

        // Добавляем UDP сокет для отслеживания входящих пакетов
        pollfd udpFd{};
        udpFd.fd = udpServer.getFd();
        udpFd.events = POLLIN;

        // Таймер для очищения сессий
        auto lastCleanup = std::chrono::steady_clock::now();

        // Главный цикл сервера
        while(!shutdownRequest.load(std::memory_order_acquire)){
            int ready = poll(&udpFd, 1, POLL_TIMEOUT_MS);

            if(ready == -1){
                if (errno == EINTR) continue;
                LOG_ERROR("Poll error: " + std::string(strerror(errno)));
                break;
            }

            // Обработка UDP пакетов
            if (ready > 0 && (udpFd.revents & POLLIN)) {
                udpServer.handler();
            }

            // Периодические задачи
            auto now = std::chrono::steady_clock::now();

            // Очистка просроченных сессий
            if (now - lastCleanup > std::chrono::seconds(SESSION_TIMEOUT_S)) {
               sessionManager.cleanTimeoutSessions();
               lastCleanup = now;
            }
        }

        // Graceful shutdown
        sessionManager.gracefulShutdown();

        // Останавливаем серверы
        httpServer.stop();
        udpServer.stop();
    }
    catch(const std::exception& e){
        if(pgw::logger::isInit){
            LOG_CRITICAL("Fatal error: {}", e.what());
            pgw::logger::shutdown();
        }
        else {
            std::cerr << "Fatal error: " << e.what() << '\n';
        }

        return 1;
    }
    return 0;
}
