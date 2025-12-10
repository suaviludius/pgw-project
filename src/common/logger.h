#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>

// Зачем запихиваем логгер в класс?
// 1) Класс статический, используем для создания
// удобаной обертки для управления логером(-ами)
// 2) Если логгер поменяется, то нужно будет
// исправить реализацию только файлов Logger.h/cpp

class Logger {
public:
    using logger = std::shared_ptr<spdlog::logger>;
    using level = spdlog::level::level_enum;
private:
    static logger m_logger;

    Logger() = default;
    ~Logger() = default;
    static level parse_level(std::string_view level);
public:
    static void init(std::string_view logFile, std::string_view logLevel);
    static void shutdown(); // Удаление созданного логгера

    // Базовые методы
    static void trace(std::string_view message);
    static void debug(std::string_view message);
    static void info(std::string_view message);
    static void warn(std::string_view message);
    static void error(std::string_view message);
    static void critical(std::string_view message);

    // Методы логики приложения
    static void session_created(std::string_view imsi);
    static void session_rejected(std::string_view imsi, std::string_view reason);
    static void session_deleted(std::string_view imsi);
    static void udp_request(std::string_view imsi, std::string_view response);
    static void http_request(std::string_view endpoint, std::string_view client_ip);

};

#endif // LOGGER_H