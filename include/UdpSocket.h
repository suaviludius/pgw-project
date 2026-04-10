#ifndef PGW_UDP_SOCKET_H
#define PGW_UDP_SOCKET_H

#include "IUdpSocket.h"
#include "SocketUtils.h"
#include "types.h"

namespace pgw {

// UDP сокет для работы с датаграммами
// Используется для приёма пакетов от абонентов
class UdpSocket : public IUdpSocket {
private:
    // Максимальный размер приемного буффера (в байтах)
    static constexpr uint16_t MAX_BYTES_RECIEVE {1024};

    // Файловый дискриптор
    int m_fd;

    // Структура адреса сокета (ip + port + ...)
    sockaddr_in m_addr;

public:
    // Создает UDP сокет в неблокирующем режиме для асинхронной работы
    UdpSocket();
    ~UdpSocket() override;

    // Запрещаем копирование (Правило пяти)
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    UdpSocket(UdpSocket&&) = delete;
    UdpSocket& operator=(UdpSocket&&) = delete;

    // Привязывает сокет к указанному IP-адресу и порту
    void bind(pgw::types::constIp_t ip, pgw::types::port_t port) override;

    // Отправляет UDP-пакет указанному адресату
    void send(std::string_view data, const sockaddr_in& addr) override;

    // Версия send() с встроенным созданием адреса
    void send(std::string_view data, pgw::types::constIp_t ip, pgw::types::port_t port) override;

    // Принимает UDP-пакет. Возвращает пустой пакет при EAGAIN/EWOULDBLOCK
    Packet receive() override;

    // Закрывает сокет, если он открыт
    void close() override;

    // Возвращает дискриптор сокета
    int getFd() const override { return m_fd; }

    // Возвращает адрес сокета
    const sockaddr_in& getAddr() const override { return m_addr; }
};

} // namespace pgw

#endif // PGW_UDP_SOCKET_H
