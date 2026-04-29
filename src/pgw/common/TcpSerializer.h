#ifndef TCP_SERIALIZER_H
#define TCP_SERIALISER_H

#include "protocol.h"

#include <nlohmann/json.hpp>

#include <optional>


namespace pgw {

class TcpSerializer{
public:
    // Из сообщения получаем бинарный буффер
    static std::vector<uint8_t> serializer(const protocol::Message& msg);

    // Из бинарного буффера получаем сообщение
    // Так как в зпросах меньше данных, чем в ответах,
    // то можно данные вначале считывать в msg и не стесняться
    // а потом данные из msg.data перевести в JSON формат методом getJsonData(msg)
    static std::optional<protocol::Message> deserializer(const std::vector<uint8_t>& buffer);

    // Из заголовка и json данных получаем сообщение
    static protocol::Message createJsonMsg(protocol::Command command,
                                           protocol::Status status,
                                           const nlohmann::json& jsonData = nlohmann::json());

    // Из сообщения извлекаем json данные
    static nlohmann::json getJsonData(const protocol::Message& msg);
};

} // namespace pgw

#endif // TCP_SERIALISER_H