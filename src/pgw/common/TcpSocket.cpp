#include "TcpSocket.h"

#include "logger.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <system_error>
#include <vector>

namespace pgw {

TcpSocket::TcpSocket(bool isListening)
    : m_fd{-1}, m_isListening{isListening} {
    m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_fd < 0) {
        throw std::runtime_error("TCP Socket creation failed: " +
                                std::string(strerror(errno)));
    }

    int reuse = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        ::close(m_fd);
        throw std::runtime_error("TCP Socket creation failed (reuse): " +
                                 std::string(strerror(errno)));
    }

    int flags = fcntl(m_fd, F_GETFL, 0);
    if(flags < 0){
        ::close(m_fd);
        throw std::runtime_error("TCP Socket creation failed (get flags): " +
                                 std::string(strerror(errno)));
    }
    if(fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0){
        ::close(m_fd);
        throw std::runtime_error("TCP Socket creation failed (non-blocking): " +
                                 std::string(strerror(errno)));
    }

    LOG_DEBUG("TCP Socket initialized (listening={})", m_isListening);
}

TcpSocket::~TcpSocket(){
    close();
}

void TcpSocket::close() {
    if(m_fd > 0) {
        ::close(m_fd);
        LOG_DEBUG("TCP Socket close");
        m_fd = -1;
    }
}

void TcpSocket::bind(pgw::types::constIp_t ip, pgw::types::port_t port){
    m_addr = SocketUtils::createAddress(ip, port);

    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr)) == -1) {
        throw std::runtime_error("TCP Socket bind failed: " +
                                 std::string(strerror(errno)));
    }

    if (m_isListening) {
        if (::listen(m_fd, SOMAXCONN) == -1) {
            throw std::runtime_error("TCP Socket listen failed: " +
                                     std::string(strerror(errno)));
        }
        LOG_INFO("TCP Socket listening on {}", SocketUtils::addrToString(m_addr));
    } else {
        LOG_INFO("TCP Socket bound to {}", SocketUtils::addrToString(m_addr));
    }
}

std::optional<std::unique_ptr<ITcpSocket>> TcpSocket::accept(){
    if (!m_isListening) {
        LOG_ERROR("TCP Socket accept called on non-listening socket");
        return std::nullopt;
    }

    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(sockaddr_in);

    int clientFd = ::accept(m_fd, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);

    if (clientFd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return std::nullopt;
        }
        LOG_ERROR("TCP Socket accept failed: {}", strerror(errno));
        return std::nullopt;
    }

    int flags = fcntl(clientFd, F_GETFL, 0);
    if(flags < 0){
        ::close(clientFd);
        throw std::runtime_error("TCP accept socket failed (get flags): " +
                                 std::string(strerror(errno)));
    }
    if(fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) < 0){
        ::close(clientFd);
        throw std::runtime_error("TCP accept socket failed (non-blocking): " +
                                 std::string(strerror(errno)));
    }

    auto clientSocket = std::make_unique<TcpSocket>(false);
    ::close(clientSocket->m_fd);
    clientSocket->m_fd = clientFd;
    clientSocket->m_addr = clientAddr;

    LOG_INFO("TCP Socket accepted connection from {}", SocketUtils::addrToString(clientAddr));

    return clientSocket;
}

void TcpSocket::connect(pgw::types::constIp_t ip, pgw::types::port_t port){
    sockaddr_in addr = SocketUtils::createAddress(ip, port);

    int result = ::connect(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    if (result < 0) {
        if (errno == EINPROGRESS) {
            LOG_INFO("TCP Socket connection in progress to {}", SocketUtils::addrToString(addr));
            return;
        }
        throw std::runtime_error("TCP Socket connect failed: " +
                                 std::string(strerror(errno)));
    }

    LOG_INFO("TCP Socket connected to {}", SocketUtils::addrToString(addr));
}

TcpSocket::Packet TcpSocket::receive(){
    std::vector<char> buffer(MAX_BYTES_RECEIVE);
    sockaddr_in addr{};
    socklen_t addrLen = sizeof(sockaddr_in);

    if (getsockname(m_fd, reinterpret_cast<sockaddr*>(&addr), &addrLen) < 0) {
        addr = m_addr;
    }

    ssize_t bytesReceived = ::recv(m_fd, buffer.data(), buffer.size(), 0);

    if (bytesReceived < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return Packet{"", addr};
        }
        throw std::runtime_error("TCP Socket receive failed: " +
                                 std::string(strerror(errno)));
    }

    if (bytesReceived == 0) {
        LOG_INFO("TCP Socket connection closed by peer");
        return Packet{"", addr};
    }

    buffer.resize(bytesReceived);
    std::string data(buffer.begin(), buffer.end());

    LOG_TRACE("Received {} bytes on TCP socket: {}", bytesReceived, SocketUtils::addrToString(addr));

    return Packet{std::move(data), addr};
}

void TcpSocket::send(std::string_view data){
    ssize_t bytesSent = ::send(m_fd, data.data(), data.size(), 0);

    if (bytesSent < 0) {
        throw std::runtime_error("TCP Socket send failed: " +
                                 std::string(strerror(errno)));
    }

    LOG_TRACE("Sent {} bytes on TCP socket", bytesSent);
}

} // namespace pgw
