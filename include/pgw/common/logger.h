#ifndef LOGGER_H
#define LOGGER_H

#ifdef LOG_OFF

    // Полное игнорирование
    #define LOG_INFO(msg, ...)      ((void)0)
    #define LOG_DEBUG(msg, ...)     ((void)0)
    #define LOG_ERROR(msg, ...)     ((void)0)
    #define LOG_WARN(msg, ...)      ((void)0)
    #define LOG_TRACE(msg, ...)     ((void)0)
    #define LOG_CRITICAL(msg, ...)  std::cout << "[CRIT]  " << msg << std::endl

#else

#include <spdlog/spdlog.h>

namespace pgw {
namespace logger {

inline constexpr size_t FILE_SIZE {10 * 1024 * 1024};
inline constexpr size_t FILES_COUNT {3};
inline constexpr const char* PATTERN {"[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v"};

using level = spdlog::level::level_enum;

// Инициализация работы оггера
void init(std::string_view logLevel);

// Добавление файлового sink'а
void addFileSink(const std::string& logFile);

// Остановка логгера и сброс всех сообщений
void shutdown();

// Инициализирован ли логгер?
bool isInit();

// Превращение std::string_view в spdlog::level::level_enum
level parse_level(std::string_view level);

// Установка уровня логирования
void set_level(spdlog::level::level_enum level);

} // namespace logger
} // namespace pgw

// Макросы для логирования
#define LOG_TRACE(...)    spdlog::trace(__VA_ARGS__)
#define LOG_DEBUG(...)    spdlog::debug(__VA_ARGS__)
#define LOG_INFO(...)     spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)     spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)    spdlog::error(__VA_ARGS__)
#define LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)

#endif

#endif // LOGGER_H
