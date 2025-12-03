#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <list>
#include <chrono>

//  ATTENTION
//  В случае замены типа std::string на иной, необходимо также изменить
//  тип в файле constants.h на соответсвтующий.

namespace pgw::types {
    using Port = uint16_t;          // 0-65535 - достаточно uint16_t
    using Ip = std::string;
    using Seconds = std::chrono::seconds;       // тип для секунд
    using Rate = uint32_t;          // Делаем больше, чтобы наверняка
    using FilePath = std::string;
    using LogLevel = std::string;
    using Blacklist = std::list<std::string>; // Не требутеся доступ по индексу, поэтому можно использовать список

    using ConstIp = std::string_view;
    using ConstFilePath = std::string_view;
    using ConstLogLevel = std::string_view;
    using ConstImsi = std::string_view;
}
#endif // TYPES_H