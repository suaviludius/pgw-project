#ifndef PGW_SOCKET_H
#define PGW_SOCKET_H

#include "ISocket.h"
#include "types.h"

class Socket : public ISocket {
private:
    // Максимальный размер приемного буффера (в байтах)
    static constexpr uint16_t MAX_BYTES_RECIEVE {1024};

    // Файловый дискриптор
    int m_fd;

    // Структура адреса сокета (ip + port + ...)
    sockaddr_in m_addr;

    // Настройка адреса (преобразование IP и порта)
    sockaddr_in createAddress(pgw::types::constIp_t ip, pgw::types::port_t port);
public:
    // Создает сокет в неблокирующем режиме для асинхронной работы
    Socket();
    ~Socket() override;

    // Привязывает сокет к указанному IP-адресу и порту
    void bind(pgw::types::constIp_t ip, pgw::types::port_t port) override;

    // Отправляет UDP-пакет указанному адресату
    void send(std::string_view data, const sockaddr_in& addres) override;

    // Версия send() с встроенным createAddress()
    void send(std::string_view data, pgw::types::constIp_t ip, pgw::types::port_t port) override;

    // Принимает UDP-пакет. Возвращает пустой пакет при EAGAIN/EWOULDBLOCK
    Packet receive() override;

    // Закрывает сокет, если он открыт
    void close() override;

    // Возвращает дискриптор сокета
    int getFd() const override {return m_fd;}

    // Возвращает адрес сокета
    const sockaddr_in& getAddr() const override {return m_addr;}

    // Преобразует sockaddr_in в читаемую строку формата "IP:PORT"
    std::string addrToString(const sockaddr_in& addr) override;
};

#endif // PGW_SOCKET_H