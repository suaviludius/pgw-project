#ifndef PGW_TYPES_H
#define PGW_TYPES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <chrono>

//  ATTENTION
//  В случае замены типа std::string на иной, необходимо также изменить
//  тип в файле constants.h на соответсвтующий.

namespace pgw {
namespace types {

using port_t = uint16_t;                // 0-65535 - достаточно uint16_t
using ip_t = std::string;
using seconds_t = std::chrono::seconds; // Тип для секунд
using rate_t = int64_t;                 // Тип для количества сессий (в секунду) на удаление
using filePath_t = std::string;
using logLevel_t = std::string;
using imsi_t = std::string;

using constIp_t = std::string_view;
using constFilePath_t = std::string_view;
using constLogLevel_t = std::string_view;
using constImsi_t = std::string_view;

using Blacklist = std::unordered_set<pgw::types::imsi_t>;

} // namespace types
} // namespace pgw

#endif // PGW_TYPES_H
