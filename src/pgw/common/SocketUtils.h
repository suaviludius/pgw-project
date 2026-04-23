#ifndef PGW_SOCKET_UTILS_H
#define PGW_SOCKET_UTILS_H

#include "types.h"

#include <netinet/in.h>  // sockaddr_in, htons, inet_pton
#include <string>

namespace pgw {

// Утилиты для работы с сокетами
// Содержат общий код для UDP и TCP сокетов
class SocketUtils {
public:
    // Создаёт sockaddr_in структуру из IP и порта
    static sockaddr_in createAddress(pgw::types::constIp_t ip, pgw::types::port_t port);

    // Преобразует sockaddr_in в читаемую строку формата "IP:PORT"
    static std::string addrToString(const sockaddr_in& addr);

    // Преобразует строку "IP:PORT" в sockaddr_in
    static sockaddr_in parseAddress(std::string_view addrStr);
};

} // namespace pgw

#endif // PGW_SOCKET_UTILS_H