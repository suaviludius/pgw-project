#ifndef I_UDP_SOCKET_H
#define I_UDP_SOCKET_H

#include "ISocket.h"

namespace pgw {

// UDP сокет для работы с датаграммами
// Используется для приёма пакетов от абонентов
class IUdpSocket : public ISocket {
public:
    // Привязывает сокет к указанному IP-адресу и порту
    virtual void bind(pgw::types::constIp_t ip, pgw::types::port_t port) = 0;

    // Отправляет UDP-пакет указанному адресату
    virtual void send(std::string_view data, const sockaddr_in& addr) = 0;

    // Отправляет UDP-пакет по IP:port
    virtual void send(std::string_view data, pgw::types::constIp_t ip, pgw::types::port_t port) = 0;

    // Принимает UDP-пакет. Возвращает пустой пакет при отсутствии данных
    virtual Packet receive() = 0;
};

} // namespace pgw

#endif // I_UDP_SOCKET_H
