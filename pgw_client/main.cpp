#include "Config.h"
#include "logger.h"
#include "Socket.h"

#include <poll.h>
#include <atomic>
#include <chrono>
#include <iostream> // cerr

// Миллисекундный таймаут для poll()
constexpr int POLL_TIMEOUT_MS {500};

int main(int argc, char* argv[]){
    try{
        std::string configPath = "configs/pgw_client.json";
        std::string imsi = "000111222333444";

        if (argc == 2) {
            // Только IMSI: ./pgw_client 001010123456789
            imsi = argv[1];

        } else if (argc == 3) {
            // Только JSON + IMSI: ./pgw_client config.json 001010123456789
            configPath = argv[1];
            imsi = argv[2];
        }

        // Чистаем конфигурацию из JSON файла
        pgw::client::Config config(configPath);

        // Инициализируем логгер параметрами из конфиг файла
        pgw::logger::init(
            config.getLogFile(),
            config.getLogLevel()
        );

        std::unique_ptr<ISocket> socket{std::make_unique<Socket>()};
        LOG_INFO("Client send imsi: {}", imsi);
        socket->send(imsi, config.getServerIp(), config.getServerPort());

        // Добавляем UDP сокет для отслеживания входящих пакетов
        pollfd udpFd{};
        udpFd.fd = socket->getFd();
        udpFd.events = POLLIN;  // Ждём данные для чтения

        // Ждём события на UDP сокете с таймаутом POLL_TIMEOUT_MS ms
        int ready = poll(&udpFd, 1, POLL_TIMEOUT_MS);

        // Ошибка poll()
        if(ready == -1){
            //if (errno == EINTR) continue;  // Прервано сигналом
            LOG_ERROR("Poll error: " + std::string(strerror(errno)));
        }

        // Обработка UDP пакета, если есть данные
        if (ready > 0 && (udpFd.revents & POLLIN)) {
            // Обрабатываем пакет
            auto packet = socket->receive();
            LOG_INFO("Server response: {}", packet.data);
        }

        socket->close();
        LOG_INFO("Client socket closed");
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