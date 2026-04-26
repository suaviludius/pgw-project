#ifndef TCP_SERIALIZER_H
#define TCP_SERIALISER_H

#include "protocol.h"

#include <optional>

namespace pgw {

class TcpSerializer{
public:
    // Из сообщения получаем бинарный буффер
    static std::vector<uint8_t> serializer(const protocol::Message& msg);

    // Из бинарного буффера получаем сообщение
    static std::optional<protocol::Message> deserializer(const std::vector<uint8_t>& buffer);

    // Из json файла получаем сообщение
    //static protocol::Message parse(protocol::Command, protocol::Status,jsonData);
};

// TODO: Стоит реализовать метод parse сообщения из json формата в сообщение

} // namespace pgw

#endif // TCP_SERIALISER_H