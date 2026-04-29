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

// TODO: можно сделать enum с кодами ошибок, чтобы классифицировать
// по какой причине не получилось считать TCP сообщение
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

protocol::Message TcpSerializer::createJsonMsg(protocol::Command command,
                                               protocol::Status status,
                                               const nlohmann::json& jsonData){

    protocol::Message msg;
    msg.header.version = protocol::PROTOCOL_VERSION;
    msg.header.command = command;
    msg.header.status = status;

    if (!jsonData.is_null()) {
        // Преобразуем объект JSON в std::string = "..."
        std::string jsonStr = jsonData.dump();
        // Так как char это байт, то он легко кладется в буффер
        msg.data.assign(jsonStr.begin(), jsonStr.end());
        msg.header.length = msg.data.size();
    } else {
        msg.header.length = 0;
    }

    return msg;
}

nlohmann::json TcpSerializer::getJsonData(const protocol::Message& msg) {
    if (msg.data.empty()) {
        return nlohmann::json::object();  // Пустой объект
    }

    try {
        std::string jsonStr(msg.data.begin(), msg.data.end());
        return nlohmann::json::parse(jsonStr);
    } catch (const std::exception& e) {
        return nlohmann::json::object();
    }
}

} // namespace pgw
