#include "config.h"
#include "logger.h"

#include "CdrWriter.h"
#include "SessionManager.h"
#include "Socket.h"
#include "UdpServer.h"
#include "HttpServer.h"

#include <poll.h>
#include <atomic>  // Для atomic

int main(){
    try{
        // Чистаем конфигурацию из файла
        Config config("config.json");
        // Инициализируем логгер параметрами из конфиг файла
        logger::init(
                     config.getLogFile(),
                     config.getLogLevel()
        );
        // Инициализирцем журнал создания сессий
        CdrWriter cdrWriter(config.getCdrFile());
        // Инициализирцем менеджер сессий
        SessionManager sessionManager(
            config.getBlacklist(),
            config.getSessionTimeoutSec(),
            config.getGracefulShutdownRate()
        );
        // Инициализируем сервер на прием сообщений от клиента
        UdpServer udpServer(
            sessionManager,
            cdrWriter,
            config.getUdpIp(),
            config.getUdpPort()
        );
        // Инициализируем сервер на прием http запросов менеджеру сессий
        HttpServer httpServer(sessionManager, config.getHttpPort());

        // Запуск серверов
        udpServer.start();
        httpServer.start();  // Запускается в отдельном потоке

        std::atomic<bool> shutdownRequest{false}; // Запрос на завершение (принимается из любых потоков)
/*

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
        LOG_INFO("Graceful shutdown in progress...");Cmake

        // Выполняем шаг graceful shutdown
        sessionManager.gracefulShutdown();
        if (completed) {
            LOG_INFO("Graceful shutdown completed, stopping server...");
            running = false;  // Завершаем главный цикл
        }
    }

}
*/

        // Останавливаем серверы
        udpServer.stop();
        httpServer.stop();
        logger::shutdown();
    }
    catch(const std::exception& e){
        if(logger::isInit){
            LOG_CRITICAL("Fatal error: {}", e.what());
            logger::shutdown();
        }
        else {
            std::cerr << "Fatal error:" << e.what() << '\n';
        }
        return 1;
    }
    return 0;
}