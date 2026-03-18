#include "UdpSocket.h"

#include "logger.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <system_error>
#include <vector>

namespace pgw {

UdpSocket::UdpSocket(){
    m_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_fd < 0) {
        throw std::runtime_error("UDP Socket creation failed: " +
                                std::string(strerror(errno)));
    }

    int reuse = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        ::close(m_fd);
        throw std::runtime_error("UDP Socket creation failed (reuse): " +
                                 std::string(strerror(errno)));
    }

    int flags = fcntl(m_fd, F_GETFL, 0);
    if(flags < 0){
        ::close(m_fd);
        throw std::runtime_error("UDP Socket creation failed (get flags): " +
                                 std::string(strerror(errno)));
    }
    if(fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0){
        ::close(m_fd);
        throw std::runtime_error("UDP Socket creation failed (non-blocking): " +
                                 std::string(strerror(errno)));
    }

    LOG_DEBUG("UDP Socket initialized");
}

UdpSocket::~UdpSocket(){
    close();
}

void UdpSocket::close() {
    if(m_fd > 0) {
        ::close(m_fd);
        LOG_DEBUG("UDP Socket close");
        m_fd = -1;
    }
}

void UdpSocket::bind(pgw::types::constIp_t ip, pgw::types::port_t port){
    m_addr = SocketUtils::createAddress(ip, port);

    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr)) == -1) {
        throw std::runtime_error("UDP Socket bind failed: " +
                                 std::string(strerror(errno)));
    }

    LOG_INFO("UDP Socket bind {}", addrToString(m_addr));
}

UdpSocket::Packet UdpSocket::receive(){
    std::vector<char> buffer(MAX_BYTES_RECIEVE);
    sockaddr_in addr{};
    socklen_t addrLen = sizeof(sockaddr_in);

    ssize_t bytesReceived = ::recvfrom(m_fd, buffer.data(), buffer.size(), 0,
                                      reinterpret_cast<sockaddr*>(&addr), &addrLen);

    if (bytesReceived < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return Packet{"", addr};
        }
        throw std::runtime_error("UDP Socket receive failed: " +
                                 std::string(strerror(errno)));
    }

    buffer.resize(bytesReceived);
    std::string data(buffer.begin(), buffer.end());

    LOG_TRACE("Received {} bytes on UDP socket: {}", bytesReceived, addrToString(addr));

    return Packet{std::move(data), addr};
}

void UdpSocket::send(std::string_view data, const sockaddr_in& addr){
    ssize_t bytesSent = ::sendto(m_fd, data.data(), data.size(), 0,
                                 reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

    if(bytesSent < 0){
        throw std::runtime_error("UDP Socket send failed: " + std::string(strerror(errno)));
    }

    LOG_TRACE("Sent {} bytes on UDP socket: {}", bytesSent, addrToString(addr));
}

void UdpSocket::send(std::string_view data, pgw::types::constIp_t ip, pgw::types::port_t port){
    sockaddr_in addr = SocketUtils::createAddress(ip, port);
    send(data, addr);
}

std::string UdpSocket::addrToString(const sockaddr_in& addr) {
    return SocketUtils::addrToString(addr);
}

} // namespace pgw
