#ifndef I_SOCKET_H
#define I_SOCKET_H

#include "types.h"

#include <string>
#include <netinet/in.h>  // sockaddr_in для Unix систем

namespace pgw {

// Базовый интерфейс для всех сокетов
// Предоставляет минимальный набор методов для работы с poll/epoll
class ISocket {
public:
    // Пакет данных (используется в UDP и TCP)
    struct Packet {
        std::string data;       // Принятые данные
        sockaddr_in senderAddr; // Адрес отправителя (для TCP - локальный адрес)
    };

    virtual ~ISocket() = default;

    // Закрывает сокет
    virtual void close() = 0;

    // Возвращает файловый дескриптор сокета (для poll/epoll)
    virtual int getFd() const = 0;

    // Возвращает локальный адрес сокета
    virtual const sockaddr_in& getAddr() const = 0;

    // Преобразует sockaddr_in в строку "IP:PORT"
    virtual std::string addrToString(const sockaddr_in& addr) = 0;
};

} // namespace pgw

#endif // I_SOCKET_H
