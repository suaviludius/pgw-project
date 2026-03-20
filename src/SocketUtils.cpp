#include "SocketUtils.h"

#include <arpa/inet.h>  // inet_pton, inet_ntop
#include <netdb.h>      // getaddrinfo
#include <cstring>      // memset
#include <stdexcept>
#include <system_error>

namespace pgw {

sockaddr_in SocketUtils::createAddress(pgw::types::constIp_t ip, pgw::types::port_t port) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // Сначала пробуем как IP-адрес
    if (::inet_pton(AF_INET, std::string(ip).c_str(), &addr.sin_addr) == 1) {
        return addr;
    }

    // Если не IP, пробуем резолвить как hostname
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = ::getaddrinfo(std::string(ip).c_str(), nullptr, &hints, &res);
    if (status != 0) {
        throw std::runtime_error("Failed to resolve hostname: " + std::string(ip) + 
                                 " (" + std::string(gai_strerror(status)) + ")");
    }

    // Копируем первый найденный адрес
    addr.sin_addr = reinterpret_cast<struct sockaddr_in*>(res->ai_addr)->sin_addr;
    ::freeaddrinfo(res);

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