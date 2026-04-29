#ifndef PGW_PROTOCOL_H
#define PGW_PROTOCOL_H

#include <cstdint> // для uint_t
#include <vector>

namespace pgw {
namespace protocol{

using version_t = uint8_t;  // Версия протокола
using command_t = uint8_t;  // Код команды (Command)
using status_t = uint8_t;   // Код статуса (Status)
using length_t = uint32_t;  // Длина поля data в байтах (Little-Endian)
using data_t = uint8_t;     // data

// Версия протокола
constexpr version_t PROTOCOL_VERSION = 0x01;

// Минимальный размер пакета
constexpr size_t MIN_MESSAGE_SIZE = sizeof(version_t) + sizeof(command_t);

// Коды команд (запросы от клиента)
enum class Command : command_t {
    GET_STATS       = 0x01, // Получить статистику
    GET_SESSIONS    = 0x02, // Список активных сессий
    GET_CDR         = 0x03, // Получить CDR записи
    START_SESSION   = 0x04, // Cоздать сессию
    STOP_SESSION    = 0x05, // Принудительно завершить сессию
    SHUTDOWN        = 0x06, // Graceful shutdown

    TEST            = 0x77, // Команда проверки (одинокая и не нужная)
};

// Коды ответов (статусы от сервера)
enum class Status : status_t {
    OK              = 0x01, // Успешное выполнение
    ERROR           = 0x02, // Общая ошибка
    INVALID_COMMAND = 0x03, // Неправильная команда
    INVALID_PARAMS  = 0x04, // Неправильные параметры с командой
    NOT_FOUND       = 0x05, // Ресурс не найден

    TEST            = 0x77, // Статус проверки (одинокий и не нужный)
};

// Разделяем сообщение на две части: заголовок + данные,
// Это удобно при валидации и парсинге.
// 1. можно узнать сразу размер заголовка (минимальный размер сообщения)
// 2. можно обрабатывать по отдельности заголовок и данные (для автоматов хорошо),
// а можно как единое целое
// 3. В дальнейшем будет удобно отделить шифрованную часть от нешифрованной
// (если захочится работать в КГБ)

// Заголовок
#pragma pack(push,1)
struct MessageHeader {
    version_t version;
    Command command;
    Status status;
    length_t length;
};
#pragma pack(pop)

// Сообщение = заголовок + данные
struct Message {
    MessageHeader header;
    std::vector<data_t> data;  // Тело сообщения (бинарные данные или JSON)
};

} // namespace protocol
} // namespace pgw

#endif // PGW_PROTOCOL_H