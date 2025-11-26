#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <list>

//  ATTENTION
//  В случае замены типа std::string на иной, необходимо также изменить
//  тип в файле constants.h на соответсвтующий.

namespace types {
    using Port = uint16_t;          // 0-65535 - достаточно uint16_t
    using Ip = std::string;
    using Seconds = uint32_t;       // даже 100 лет влезет - достаточно uint32_t
    using Rate = uint32_t;          // Делаем больше, чтобы наверняка
    using FilePath = std::string;
    using LogLevel = std::string;
    using Blacklist = std::list<std::string>; // Не требутеся доступ по индексу, поэтому можно использовать список
}
#endif // TYPES_H