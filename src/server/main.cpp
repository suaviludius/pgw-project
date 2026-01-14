#include "UdpServer.h"
#include "HttpServer.h"
#include "SessionManager.h"
#include "CdrWriter.h"
#include "config.h"
#include "logger.h"
#include "Socket.h"

#include <poll.h>

int main(){
    try{
        Config config("config.json");
        SessionManager sessionManager(
            config.getBlacklist(),
            config.getSessionTimeoutSec(),
            config.getGracefulShutdownRate()
        );
        CdrWriter cdrWriter(config.getCdrFile());
        UdpServer udpServer(
            sessionManager,
            cdrWriter,
            config.getUdpIp(),
            config.getUdpPort()
        );
        HttpServer httpServer(sessionManager, config.getHttpPort());

        // Запуск серверов
        udpServer.start();
        httpServer.start();  // Запускается в отдельном потоке

        // Подготовка для poll()
        std::vector<pollfd> fds;
        // Добавляем UDP сокет
        pollfd udpFd{};
        udpFd.fd = udpServer.getFd();
        udpFd.events = POLLIN;  // Ждём данные для чтения
        fds.push_back(udpFd);

        // Таймер для очищения сессий
        auto lastCleanup = std::chrono::steady_clock::now();

        bool running{true};
        while(running){
            // Ждём события на UDP сокете с таймаутом 100ms
            int ready = poll(fds.data(), fds.size(), 100);
            if(ready == -1)
                if (errno == EINTR) continue;  // Прервано сигналом
                Logger::error("Poll error: " + std::string(strerror(errno)));
                break;
            }
            // Обработка UDP событий
            if (ready > 0 && (fds[0].revents & POLLIN)) {
                udpServer.handler();  // Обрабатываем все доступные пакеты
            }

            // Периодические задачи (выполняются по тайсауту)
            auto now = std::chrono::steady_clock::now();
            // 1. Очистка просроченных сессий (каждые 30 секунд)
            if (now - lastCleanup > std::chrono::seconds(30)) {
                sessionManager.cleanTimeoutSessions();
                lastCleanup = now;
            }

            // 2. Graceful shutdown (если запрошен через HTTP)
            if (sessionManager.isShutdownRequested()) {
                Logger::info("Graceful shutdown in progress...");

                // Выполняем шаг graceful shutdown
                sessionManager.gracefulShutdown();
                if (completed) {
                    Logger::info("Graceful shutdown completed, stopping server...");
                    running = false;  // Завершаем главный цикл
                }
            }

        }

        // Останавливаем серверы
        udpServer.stop();
        httpServer.stop();
    }
    catch(const std::exception& e){
        Logger::critical("Fatal error " + std::string(e.what()));
        return 0;
    }
    return 0;
}