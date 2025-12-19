#include "Socket.h"
#include "logger.h"

#include <arpa/inet.h> // inet_pton / htons / ntohs
#include <unistd.h>    // close
#include <string.h>    // memset

#include <system_error>
#include <vector>

Socket::Socket(){
    // Создаем дискриптор для сокета IPv4 + UDP
    m_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_fd < 0) {
        throw std::system_error(errno, std::system_category(), "Socket create failed (socket()): ");
    }
    // Включаем повторное использование порта
    int reuse = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        throw std::system_error(errno, std::system_category(), "Socket create failed (setsockopt() for reusing port): ");
    }
    Logger::debug("Socket created");
}

Socket::~Socket(){
    close();
}

void Socket::close() {
    if(m_fd > 0) {
        ::close(m_fd);
        Logger::debug("Socket close");
        m_fd = -1;
    }
}

void Socket::bind(pgw::types::ConstIp ip, pgw::types::Port port){
    sockaddr_in addr{}; // Структура адреса сокета (ip + port + ...)
    memset(&addr, 0 ,sizeof(sockaddr_in)); // Перед использованием нужно обнулить структуру
    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = htons(port); // Преобразует порядок байт в сетевой (big-endian)

    // Заносим IP в структуру адреса
    if (::inet_pton(AF_INET, std::string(ip).c_str(), &addr.sin_addr) != 1) {
        throw std::runtime_error("Invalid IP address: " + std::string(ip));
    }

    // Связываем сокет с адресом сокета
    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to bind socket to " + std::string(ip) + ":" + std::to_string(port));
    }

    Logger::info("Socket bind " + addrToString(addr));
    //m_socket = reinterpret_cast<struct sockaddr* > (&addr); // Для работ с функциями
}

Socket::Packet Socket::recieve(){
    std::vector<char> buffer(MAX_BYTES_RECIEVE); // Буффер для приема входных байт
    sockaddr_in addr{}; // Адрес отправителя
    socklen_t addrLen { sizeof(sockaddr_in) }; // Сохраняем размер адреса под особым типом, для работы библиотечных методов
    ssize_t bytesReceived = ::recvfrom(m_fd, buffer.data(), buffer.size(), 0,
                                      reinterpret_cast<sockaddr*>(&addr), &addrLen);
    if(bytesReceived < 0){
        throw std::system_error(errno, std::system_category(), "Socket failed received bytes");
    }

    buffer.resize(bytesReceived); // Изменяем размер вектора до фактического количества принятых байт
    std::string data(buffer.begin(), buffer.end());

    Logger::trace("Received " + std::to_string(bytesReceived) + " bytes on socket: " + addrToString(addr));

    return Packet{std::move(data), addr};
}

bool Socket::send(std::string_view data, const sockaddr_in& addr){
    ssize_t bytesSent = ::sendto(m_fd, data.data(), data.size(), 0,
                                 reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if(bytesSent < 0){
        throw std::system_error(errno, std::system_category(), "Failed to sent data");
    }

    Logger::trace("Sent " + std::to_string(bytesSent) + " bytes on socket: " + addrToString(addr));
}

std::string Socket::addrToString(const sockaddr_in& addr) {
    char ip[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip)); // Преобразуем ip-адрес в строковой формат
    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}