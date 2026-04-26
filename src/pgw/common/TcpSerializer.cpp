#include "TcpSerializer.h"

#include <cstring> // memcpy

namespace pgw {

std::vector<uint8_t> TcpSerializer::serializer(const protocol::Message& msg){
    // Создаем бинарный буффер
    std::vector<uint8_t> buffer;

    // Указатель для побайтного чтения заголовка
    const uint8_t* header = reinterpret_cast<const uint8_t*>(&msg.header);

    // Вставляем данные в буффер
    buffer.insert(buffer.end(),header, header + sizeof(protocol::MessageHeader));
    buffer.insert(buffer.end(),msg.data.begin(),msg.data.end());

    // ↑↑↑ Это сработает, если данные в буффере будут uint8_t типа
    return buffer;
}

std::optional<protocol::Message> TcpSerializer::deserializer(const std::vector<uint8_t>& buffer){
    if(buffer.size() < sizeof(protocol::MessageHeader)){
        return std::nullopt;
    }

    protocol::Message msg;

    // Копируем заголовок
    std::memcpy(&msg.header, buffer.data(), sizeof(protocol::MessageHeader));

    // Проверяем версию
    if(msg.header.version != protocol::PROTOCOL_VERSION){
        return std::nullopt;
    }

    // Проверяем размер буффера
    if(buffer.size() < sizeof(protocol::MessageHeader) + msg.header.length){
        return std::nullopt;
    }

    // Копируем данные в структуру
    msg.data.resize(msg.header.length);
    if(msg.header.length > 0){
        std::memcpy(msg.data.data(), buffer.data() + sizeof(protocol::MessageHeader), msg.header.length);
    }

    return msg;
}

/*
protocol::Message parse(protocol::Command, protocol::Status, jsonData){
    Message msg;
    return msg;
}
*/

} // namespace pgw
