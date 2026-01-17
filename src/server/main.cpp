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
        CdrWriter cdrWriter(
            config.getCdrFile()
        );
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
        // Запрос на завершение (принимается из любых потоков)
        std::atomic<bool> shutdownRequest{false};
        // Инициализируем сервер на прием http запросов менеджеру сессий
        HttpServer httpServer(
            sessionManager,
            config.getHttpPort(),
            shutdownRequest
        );

        // Запуск серверов
        udpServer.start();
        httpServer.start();  // Запускается в отдельном потоке
        // Браузер:
        // http://localhost:8080/check_subscriber?imsi=250990123456789
        // Командная строка(curl в помощь):
        // curl -X GET "http://localhost:8080/check_subscriber?imsi=123456789012345"
        // curl -X POST http://localhost:8080/stop

        // Подготовка для poll()
        std::vector<pollfd> fds;
        // Добавляем UDP сокет
        pollfd udpFd{};
        udpFd.fd = udpServer.getFd();
        udpFd.events = POLLIN;  // Ждём данные для чтения
        fds.push_back(udpFd);

        // Таймер для очищения сессий
        auto lastCleanup = std::chrono::steady_clock::now();

        // Выполняем работу сервера пока не придет запрос на shutdown request
        while(!shutdownRequest.load(std::memory_order_acquire)){
            // Ждём события на UDP сокете с таймаутом 100ms
            int ready = poll(fds.data(), fds.size(), 100);
            if(ready == -1){
                if (errno == EINTR) continue;  // Прервано сигналом
                LOG_ERROR("Poll error: " + std::string(strerror(errno)));
                break;
            }
            // Обработка UDP событий
            if (ready > 0 && (fds[0].revents & POLLIN)) {
                // Обрабатываем все доступные пакеты
                udpServer.handler();
            }

            // Периодические задачи (выполняются по таймауту)
            auto now = std::chrono::steady_clock::now();
            // 1. Очистка просроченных сессий (каждые 30 секунд)
            if (now - lastCleanup > std::chrono::seconds(30)) {
                sessionManager.cleanTimeoutSessions();
                lastCleanup = now;
            }
            // 2. Сюда можно бахнуть еще задач
        }
        // Удаляем сессии в менеджере
        sessionManager.gracefulShutdown();
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