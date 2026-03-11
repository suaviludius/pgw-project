#ifndef PGW_SOCKET_FACTORY_H
#define PGW_SOCKET_FACTORY_H

#include "ISocket.h"
#include "IUdpSocket.h"
#include "ITcpSocket.h"
#include "types.h"

#include <memory>
#include <string_view>

namespace pgw {

// Типы сокетов для создания
enum class SocketType {
    UDP,  // Датаграммный сокет (для пакетов абонентов)
    TCP   // Потоковый сокет (для команд управления)
};

// Преобразует строку в тип сокета
SocketType parseSocketType(std::string_view type);

// Преобразует тип сокета в строку
std::string_view socketTypeToString(SocketType type);

// Фабрика для создания сокетов
class SocketFactory {
public:
    // Создает UDP сокет
    static std::unique_ptr<IUdpSocket> createUdp();

    // Создает TCP сокет
    // isListening: true = серверный сокет, false = клиентский
    static std::unique_ptr<ITcpSocket> createTcp(bool isListening = false);

    // Создает сокет типа ISocket (для общего использования с poll)
    // Возвращает базовый тип, но фактически создает конкретную реализацию
    static std::unique_ptr<ISocket> create(SocketType type, bool isListening = false);
};

} // namespace pgw

#endif // PGW_SOCKET_FACTORY_H
