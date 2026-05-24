#include "SocketFactory.h"
#include "UdpSocket.h"
#include "TcpSocket.h"
#include "logger.h"

#include <stdexcept>

namespace pgw {

SocketType parseSocketType(std::string_view type) {
    if (type == "udp" || type == "UDP") {
        return SocketType::UDP;
    }
    if (type == "tcp" || type == "TCP") {
        return SocketType::TCP;
    }
    throw std::invalid_argument("Unknown socket type: " + std::string(type));
}

std::string_view socketTypeToString(SocketType type) {
    switch (type) {
        case SocketType::UDP: return "udp";
        case SocketType::TCP: return "tcp";
        default: return "unknown";
    }
}

std::unique_ptr<IUdpSocket> SocketFactory::createUdp() {
    LOG_INFO("Creating UDP socket");
    return std::make_unique<UdpSocket>();
}

std::unique_ptr<ITcpSocket> SocketFactory::createTcp(bool isListening) {
    LOG_INFO("Creating TCP socket (listening={})", isListening);
    return std::make_unique<TcpSocket>(isListening);
}

std::unique_ptr<ISocket> SocketFactory::create(SocketType type, bool isListening) {
    switch (type) {
        case SocketType::UDP:
            return createUdp();
        case SocketType::TCP:
            return createTcp(isListening);
        default:
            throw std::invalid_argument("Unknown socket type");
    }
}

} // namespace pgw
