#ifndef LOGGER_H
#define LOGGER_H

#include "config.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h> // Для записи логов в файл
#include <spdlog/sinks/stdout_color_sinks.h> // Для вывода логов в консоль

// Зачем запихиваем логгер в класс?
// 1) Класс статический, используем для создания
// удобаной обертки для управления логером(-ами)
// 2) Если логгер поменяется, то нужно будет
// исправить реализацию только файлов Logger.h/cpp

class Logger {
public:
    using LogPtrType = std::shared_ptr<spdlog::logger>;
    using LogLevelType = spdlog::level::level_enum;
private:
    static LogPtrType m_logger;
    static bool m_initialized;

    Logger() = default;
    ~Logger() = default;
    static LogLevelType parse_level(const std::string& level);
public:
    static void init(const Config& config);

    // Базовые методы
    static void trace(const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void critical(const std::string& message);

    // Методы логики приложения
    static void session_created(const std::string& imsi);
    static void session_rejected(const std::string& imsi, const std::string& reason);
    static void session_deleted(const std::string& imsi);
    static void udp_request(const std::string& imsi, const std::string& response);
    static void http_request(const std::string& endpoint, const std::string& client_ip);

};

#endif // LOGGER_H