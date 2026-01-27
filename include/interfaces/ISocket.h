#ifndef I_SOCKET_H
#define I_SOCKET_H

#include "types.h"

#include <netinet/in.h> // Структуры сокетов для Unix систем

class ISocket {
public:
    struct Packet{
        std::string data;       // Пришедшие данные
        sockaddr_in senderAddr; // Адрес отправителя (также адрес назначения при отправке)
    };

    virtual ~ISocket() = default;

    virtual void bind(pgw::types::constIp_t ip, pgw::types::port_t port) = 0;
    virtual void send(std::string_view data, const sockaddr_in& addres) = 0;
    virtual void send(std::string_view data, pgw::types::constIp_t ip, pgw::types::port_t port) = 0;
    virtual Packet receive() = 0;
    virtual void close() = 0;

    virtual int getFd() const = 0;
    virtual const sockaddr_in& getAddr() const = 0;
    virtual std::string addrToString(const sockaddr_in& addr) = 0; // Из sockaddr_in делает string c ip:port
};

#endif // I_SOCKET_H