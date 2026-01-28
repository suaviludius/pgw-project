#include "logger.h"
#include "Config.h"
#include "CdrWriter.h"
#include "SessionManager.h"
#include "Socket.h"
#include "UdpServer.h"
#include "HttpServer.h"

#include <poll.h>
#include <atomic>
#include <chrono>
#include <iostream> // cerr

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

        // Чистаем конфигурацию из JSON файла
        pgw::server::Config config(configPath);

        // Инициализируем логгер параметрами из конфиг файла
        pgw::logger::init(
            config.getLogFile(),
            config.getLogLevel()
        );

        // Инициализирцем журнал создания сессий
        CdrWriter cdrWriter(
            config.getCdrFile()
        );

        // Инициализирцем менеджер сессий
        SessionManager sessionManager(
            cdrWriter,
            config.getBlacklist(),
            config.getSessionTimeoutSec(),
            config.getGracefulShutdownRate()
        );

        // Инициализируем сервер на прием сообщений от клиента
        UdpServer udpServer(
            sessionManager,
            config.getUdpIp(),
            config.getUdpPort()
        );

        // Флаг запроса на завершение работы (разделяемый между потоками)
        std::atomic<bool> shutdownRequest{false};

        // Инициализируем сервер на прием http запросов менеджеру сессий
        HttpServer httpServer(
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

        // Выполняем работу сервера пока не придет запрос на shutdown request
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

            // Каждые POLL_TIMEOUT_MS миллисекунд при отсутсвии данных на сокете (ready == 0)
            // sessionManager.cleanTimeoutSessions();

            // Периодические задачи (выполняются по таймауту)
            auto now = std::chrono::steady_clock::now();

            // Очистка просроченных сессий (каждые SESSION_TIMEOUT_S секунд)
            if (now - lastCleanup > std::chrono::seconds(SESSION_TIMEOUT_S)) {
               sessionManager.cleanTimeoutSessions();
               lastCleanup = now;
            }

            // Сюда можно бахнуть еще задач
            // . . .
        }

        // Удаляем сессии в менеджере
        sessionManager.gracefulShutdown();

        // Останавливаем работу серверов
        httpServer.stop();
        udpServer.stop();
    }
    catch(const std::exception& e){
        if(pgw::logger::isInit){
            // Критическая ошибка при выполнении
            LOG_CRITICAL("Fatal error: {}", e.what());
            pgw::logger::shutdown();
        }
        else {
            // Логгер не инициализирован - вывод в stderr
            std::cerr << "Fatal error:" << e.what() << '\n';
        }

        return 1;
    }
    return 0;
}