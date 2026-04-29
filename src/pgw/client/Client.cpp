#include "Client.h"
#include "Config.h"
#include "SocketFactory.h"
#include "logger.h"

#include <poll.h>
#include <atomic>
#include <chrono>
#include <iostream> // cerr


namespace pgw {
namespace client {

// Миллисекундный таймаут для poll()
constexpr int POLL_TIMEOUT_MS {500};

Client::Client(const pgw::types::filePath_t& configPath, const pgw::types::imsi_t& imsi)
    : m_config{std::make_unique<Config>(configPath)}, m_imsi{imsi}, m_socket{pgw::SocketFactory::createUdp()} {
}


Client::~Client() {
    if (pgw::logger::isInit()) {
        pgw::logger::shutdown();
    }
}


int Client::run(){
    try {
        // Проверка валидности конфигурации
        if (!m_config->isValid()) {
            std::cerr << "Config error: " << m_config->getError() << '\n';
            return 1;
        }

        // Проверка валидности созданного соккета
        if (!m_socket) {
            std::cerr << "Failed to create socket\n";
            return 1;
        }

        // Инициализация логгера
        if (!initializeLogger()) {
            std::cerr << "Failed to initialize logger\n";
            return 1;
        }

        LOG_INFO("Client started successfully");

        // Запуск основного цикла обработки событий
        runEventLoop();

        LOG_INFO("Client stopped gracefully");
        return 0;
    }
    catch (const std::exception& e) {
        if (pgw::logger::isInit()) {
            LOG_CRITICAL("Fatal error: {}", e.what());
        } else {
            std::cerr << "Fatal error: " << e.what() << '\n';
        }
        return 1;
    }
}

bool Client::initializeLogger() {
    pgw::logger::init(m_config->getLogLevel());
    pgw::logger::addFileSink(std::string(m_config->getLogFile()));
    return pgw::logger::isInit();
}


void Client::runEventLoop(){

    // Отправляем сообщение через соккет
    m_socket->send(m_imsi, m_config->getServerIp(), m_config->getServerPort());

    // Настройка poll для отслеживания UDP сокета
    pollfd udpFd{};
    udpFd.fd = m_socket->getFd();
    udpFd.events = POLLIN;  // Ждём данные для чтения

    // Ждём события на UDP сокете с таймаутом POLL_TIMEOUT_MS ms
    int ready = poll(&udpFd, 1, POLL_TIMEOUT_MS);

    // Ошибка poll()
    if(ready == -1){
        LOG_ERROR("Poll error: {}", strerror(errno));
    }

    // Обработка UDP пакета, если есть данные
    if (ready > 0 && (udpFd.revents & POLLIN)) {
        // Обрабатываем пакет
        auto packet = m_socket->receive();
        LOG_INFO("Server response: {}", packet.data);
    }

    m_socket->close();
    LOG_INFO("Client socket closed");
}

} // namespace client
} // namespace pgw
