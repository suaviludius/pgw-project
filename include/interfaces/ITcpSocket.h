#ifndef I_TCP_SOCKET_H
#define I_TCP_SOCKET_H

#include "ISocket.h"

#include <optional>
#include <memory>

namespace pgw {

// TCP сокет для надёжной передачи данных
// Используется для будущих команд управления (SCADA, мониторинг)
class ITcpSocket : public ISocket {
public:
    // Привязывает сокет к указанному IP-адресу и порту
    // Для серверного сокета: также вызывает listen()
    virtual void bind(pgw::types::constIp_t ip, pgw::types::port_t port) = 0;

    // Принимает входящее соединение (только для серверного сокета)
    // Возвращает новый сокет для клиента или nullopt если нет соединений
    // Гениальная вещь которую надо попробовать
    virtual std::optional<std::unique_ptr<ITcpSocket>> accept() = 0;

    // Устанавливает соединение с удалённым сервером (для клиентского сокета)
    virtual void connect(pgw::types::constIp_t ip, pgw::types::port_t port) = 0;

    // Отправляет данные через установленное соединение
    virtual void send(std::string_view data) = 0;

    // Принимает данные. Возвращает пустой пакет при отсутствии данных
    virtual Packet receive() = 0;

    // Проверяет, является ли сокет серверным (listening)
    virtual bool isListening() const = 0;
};

} // namespace pgw

#endif // I_TCP_SOCKET_H