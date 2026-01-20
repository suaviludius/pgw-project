#include "server/Socket.h"

#include "common/logger.h"

#include <arpa/inet.h> // inet_pton / htons / ntohs

#include <unistd.h>    // close
#include <string.h>    // memset
#include <fcntl.h>     // fcntl
#include <system_error>
#include <vector>

Socket::Socket(){
    // Создаем дискриптор для сокета IPv4 + UDP
    m_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_fd < 0) {
        throw std::runtime_error("Socket creation failed: " +
                                std::string(strerror(errno)));
    }

    // Включаем повторное использование порта
    int reuse = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        ::close(m_fd);
        throw std::runtime_error("Socket creation failed (reuse): " +
                                 std::string(strerror(errno)));
    }

    // Устанавливаем неблокирующий режим для асинхронной работы
    // (плюс poll() нужен non-blocking socket)
    int flags = fcntl(m_fd, F_GETFL, 0);
    if(flags < 0){
        ::close(m_fd);
        throw std::runtime_error("Socket creation failed (get flags): " +
                                 std::string(strerror(errno)));
    }
    if(fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0){
        ::close(m_fd);
        throw std::runtime_error("Socket creation failed (non-blocking): " +
                                 std::string(strerror(errno)));
    }
    // Примечание: Таймауты на прием не используются для максимальной производительности

    LOG_DEBUG("Socket created");
}

Socket::~Socket(){
    close();
}

void Socket::close() {
    if(m_fd > 0) {
        ::close(m_fd);
        LOG_DEBUG("Socket close");
        m_fd = -1; // Инвалидируем дескриптор
    }
}

void Socket::bind(pgw::types::ConstIp ip, pgw::types::Port port){
    sockaddr_in addr{};                     // Структура адреса сокета (ip + port + ...)
    memset(&addr, 0 ,sizeof(sockaddr_in));  // Перед использованием нужно обнулить структуру
    addr.sin_family = AF_INET;              // IPv4
    addr.sin_port = htons(port);            // Преобразует порядок байт в сетевой (big-endian)

    // Преобразуем строковый IP в бинарный формат
    if (::inet_pton(AF_INET, std::string(ip).c_str(), &addr.sin_addr) != 1) {
        throw std::runtime_error("Socket bind failed to ip address " +
                                 std::string(ip));
    }

    // Привязываем сокет к адресу
    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        throw std::runtime_error("Socket bind failed: " +
                                 std::string(strerror(errno)));
    }

    LOG_INFO("Socket bind " + addrToString(addr));
}

Socket::Packet Socket::receive(){
    std::vector<char> buffer(MAX_BYTES_RECIEVE);    // Буффер для приема входных байт
    sockaddr_in addr{};                             // Адрес отправителя
    socklen_t addrLen { sizeof(sockaddr_in) };      // Сохраняем размер адреса под особым типом, для работы библиотечных методов

    // Принимаем данные с информацией об отправителе
    ssize_t bytesReceived = ::recvfrom(m_fd, buffer.data(), buffer.size(), 0,
                                      reinterpret_cast<sockaddr*>(&addr), &addrLen);

    if (bytesReceived < 0) {
        // В неблокирующем режиме EAGAIN/EWOULDBLOCK означает "нет данных"
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            //LOG_TRACE("No data available (non-blocking socket)");
            // Возвращаем пустой пакет с адресом отправителя
            return Packet{"", addr};
        }
        else {
            throw std::runtime_error("Socket receive failed: " +
                                     std::string(strerror(errno)));
        }
    }

    // Оптимизируем буфер под фактический размер данных
    buffer.resize(bytesReceived);
    std::string data(buffer.begin(), buffer.end());

    LOG_TRACE("Received {}" + std::to_string(bytesReceived) +
              " bytes on socket: " + addrToString(addr));

    return Packet{std::move(data), addr};
}

void Socket::send(std::string_view data, const sockaddr_in& addr){
    ssize_t bytesSent = ::sendto(m_fd, data.data(), data.size(), 0,
                                 reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

    if(bytesSent < 0){
        throw std::runtime_error("Socket failed to sent data: " + std::string(strerror(errno)));
    }

    LOG_TRACE("Sent " + std::to_string(bytesSent) + " bytes on socket: " + addrToString(addr));
}

std::string Socket::addrToString(const sockaddr_in& addr) {
    char ip[INET_ADDRSTRLEN];

    // Преобразуем бинарный IP в строку
    if (::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip)) == nullptr) {
        return "Invalid address";
    }

    return std::string(ip) + " : " + std::to_string(ntohs(addr.sin_port));
}