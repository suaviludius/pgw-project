#include "SocketUtils.h"

#include <arpa/inet.h>  // inet_pton, inet_ntop
#include <cstring>      // memset
#include <stdexcept>

namespace pgw {

sockaddr_in SocketUtils::createAddress(pgw::types::constIp_t ip, pgw::types::port_t port) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (::inet_pton(AF_INET, std::string(ip).c_str(), &addr.sin_addr) != 1) {
        throw std::runtime_error("Failed to parse IP address: " + std::string(ip));
    }

    return addr;
}

std::string SocketUtils::addrToString(const sockaddr_in& addr) {
    char ip[INET_ADDRSTRLEN];

    if (::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip)) == nullptr) {
        return "Invalid address";
    }

    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

sockaddr_in SocketUtils::parseAddress(std::string_view addrStr) {
    // Ожидаемый формат: "IP:PORT"
    auto colonPos = addrStr.find(':');
    if (colonPos == std::string_view::npos) {
        throw std::invalid_argument("Invalid address format: " + std::string(addrStr));
    }

    std::string ip(addrStr.substr(0, colonPos));
    std::string portStr(addrStr.substr(colonPos + 1));

    pgw::types::port_t port = static_cast<pgw::types::port_t>(std::stoi(portStr));

    return createAddress(ip, port);
}

} // namespace pgw