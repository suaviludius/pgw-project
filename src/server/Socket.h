#ifndef SOCKET_H
#define SOCKET_H

#include "types.h"

#include <netinet/in.h> // структуры сокетов

class Socket {
public:
    static constexpr uint16_t MAX_BYTES_RECIEVE {1024};
private:
    int m_fd; // file diskriptor
public:
    Socket();
    ~Socket();

    struct Packet{
        std::string data;       // Тут будет лежать imsi
        sockaddr_in senderAddr; // Этот адрес используем для send() внутри сервера
    };

    void bind(pgw::types::ConstIp ip, pgw::types::Port port);
    bool send(std::string_view data, const sockaddr_in& addres);
    Packet recieve();
    void close();

    std::string addrToString(const sockaddr_in& addr); // Из sockaddr_in делает string c ip:port
};

#endif // SOCKET_H