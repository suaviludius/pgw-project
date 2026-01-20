#include "common/config.h"
#include "common/logger.h"
#include "server/CdrWriter.h"
#include "server/SessionManager.h"
#include "server/Socket.h"
#include "server/UdpServer.h"
#include "server/HttpServer.h"

#include <poll.h>
#include <atomic>
#include <chrono>
#include <iostream> // cerr

// Миллисекундный таймаут для poll()
constexpr int POLL_TIMEOUT_MS {100};

int main(){
    try{
        // Чистаем конфигурацию из JSON файла
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

        // Подготовка для poll()
        std::vector<pollfd> fds;

        // / Добавляем UDP сокет для отслеживания входящих пакетов
        pollfd udpFd{};
        udpFd.fd = udpServer.getFd();
        udpFd.events = POLLIN;  // Ждём данные для чтения
        fds.push_back(udpFd);

        // Таймер для очищения сессий
        auto lastCleanup = std::chrono::steady_clock::now();

        // Выполняем работу сервера пока не придет запрос на shutdown request
        while(!shutdownRequest.load(std::memory_order_acquire)){
            // Ждём события на UDP сокете с таймаутом 100ms
            int ready = poll(fds.data(), fds.size(), POLL_TIMEOUT_MS);

            if(ready == -1){
                if (errno == EINTR) continue;  // Прервано сигналом
                LOG_ERROR("Poll error: " + std::string(strerror(errno)));
                break;
            }

            // Обработка UDP пакетов, если есть данные
            if (ready > 0 && (fds[0].revents & POLLIN)) {
                // Обрабатываем все доступные пакеты
                udpServer.handler();
            }

            // Периодические задачи (выполняются по таймауту)
            auto now = std::chrono::steady_clock::now();

            // Очистка просроченных сессий (каждые 30 секунд)
            if (now - lastCleanup > std::chrono::seconds(30)) {
                sessionManager.cleanTimeoutSessions();
                lastCleanup = now;
            }

            // Сюда можно бахнуть еще задач
            // . . .
        }

        // Удаляем сессии в менеджере
        sessionManager.gracefulShutdown();
    }
    catch(const std::exception& e){
        if(logger::isInit){
            // Критическая ошибка при выполнении
            LOG_CRITICAL("Fatal error: {}", e.what());
            logger::shutdown();
        }
        else {
            // Логгер не инициализирован - вывод в stderr
            std::cerr << "Fatal error:" << e.what() << '\n';
        }

        return 1;
    }
    return 0;
}