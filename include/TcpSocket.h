#ifndef PGW_TCP_SOCKET_H
#define PGW_TCP_SOCKET_H

#include "ITcpSocket.h"
#include "SocketUtils.h"
#include "types.h"

#include <optional>

namespace pgw {

// TCP сокет для надёжной передачи данных
// Используется для команд управления (SCADA, мониторинг)
class TcpSocket : public ITcpSocket {
private:
    // Максимальный размер приемного буффера (в байтах)
    static constexpr uint16_t MAX_BYTES_RECEIVE {4096};

    // Файловый дискриптор
    int m_fd;

    // Структура адреса сокета (ip + port + ...)
    sockaddr_in m_addr;

    // Флаг серверного сокета (слушающий)
    bool m_isListening;

public:
    // Создает TCP сокет
    // isListening: true = серверный сокет (listen), false = клиентский
    explicit TcpSocket(bool isListening = false);
    ~TcpSocket() override;

    // Запрещаем копирование (Правило пяти)
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&&) = delete;
    TcpSocket& operator=(TcpSocket&&) = delete;

    // Привязывает сокет к указанному IP-адресу и порту
    void bind(pgw::types::constIp_t ip, pgw::types::port_t port) override;

    // Принимает входящее соединение (только для серверного сокета)
    std::optional<std::unique_ptr<ITcpSocket>> accept() override;

    // Устанавливает соединение с удалённым сервером (для клиентского сокета)
    void connect(pgw::types::constIp_t ip, pgw::types::port_t port) override;

    // Отправляет данные через установленное соединение
    void send(std::string_view data) override;

    // Принимает данные. Возвращает пустой пакет при EAGAIN/EWOULDBLOCK
    Packet receive() override;

    // Закрывает сокет, если он открыт
    void close() override;

    // Возвращает дискриптор сокета
    int getFd() const override { return m_fd; }

    // Возвращает адрес сокета
    const sockaddr_in& getAddr() const override { return m_addr; }

    // Преобразует sockaddr_in в читаемую строку формата "IP:PORT"
    std::string addrToString(const sockaddr_in& addr) override;

    // Проверяет, является ли сокет серверным (listening)
    bool isListening() const override { return m_isListening; }
};

} // namespace pgw

#endif // PGW_TCP_SOCKET_H