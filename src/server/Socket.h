#ifndef PGW_SOCKET_H
#define PGW_SOCKET_H

#include "server/interfaces/ISocket.h"
#include "common/types.h"

class Socket : public ISocket {
private:
    // Максимальный размер приемного буффера (в байтах)
    static constexpr uint16_t MAX_BYTES_RECIEVE {1024};

    // Файловый дискриптор
    int m_fd;

public:
    Socket();
    ~Socket() override;

    // Привязывает сокет к указанному IP-адресу и порту
    void bind(pgw::types::ConstIp ip, pgw::types::Port port) override;

    // Отправляет UDP-пакет указанному адресату
    void send(std::string_view data, const sockaddr_in& addres) override;

    // Принимает UDP-пакет
    Packet receive() override;

    // Закрывает сокет, если он открыт
    void close() override;

    // Возвращает дискриптор сокета
    int getFd() const override {return m_fd;}

    // Преобразует sockaddr_in в читаемую строку формата "IP:PORT"
    std::string addrToString(const sockaddr_in& addr) override;
};

#endif // PGW_SOCKET_H