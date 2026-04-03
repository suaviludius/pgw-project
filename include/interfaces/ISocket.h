#ifndef I_SOCKET_H
#define I_SOCKET_H

#include "types.h"

#include <netinet/in.h> // Структуры сокетов для Unix систем

struct ISocket {
    struct Packet{
        std::string data;       // Пришедшие данные
        sockaddr_in senderAddr; // Адрес отправителя (также адрес назначения при отправке)
    };

    virtual ~ISocket() = default;

    // Закрывает сокет
    virtual void close() = 0;

    // Возвращает файловый дескриптор сокета (для poll/epoll)
    virtual int getFd() const = 0;

    // Возвращает локальный адрес сокета
    virtual const sockaddr_in& getAddr() const = 0;
};

#endif // I_SOCKET_H