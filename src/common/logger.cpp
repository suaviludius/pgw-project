#include "logger.h"

#include <spdlog/sinks/rotating_file_sink.h> // Для записи логов в файл
#include <spdlog/sinks/stdout_color_sinks.h> // Для цветного вывода логов в консоль

#include <iostream> // Для использования std::cerr
#include <map>

Logger::logger Logger::m_logger = nullptr;

void Logger::init(std::string_view logFile, std::string_view logLevel){
    if (m_logger) {
        return; // Уже инициализирован
    }
    try{
        // Определяем приемники логов
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            static_cast<std::string>(logFile),
            1024*1024*5, // 5 MB максимальный размер файл
            3,           // Хранить 3 ротированный файл
            false        // Не ротировать при открытии
        );
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        // Создаем логгер
        m_logger = std::make_shared<spdlog::logger>("pgw");
        m_logger->sinks().push_back(file_sink);
        m_logger->sinks().push_back(console_sink);
        m_logger->set_level(parse_level(logLevel));
        m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

        spdlog::set_default_logger(m_logger);

        info("Logger initialized successfully");
    }
    catch(const std::exception& e){
        shutdown();
        std::cerr << "Logger initialization failed: " << e.what() << '\n';
        throw;
    }
}

Logger::level Logger::parse_level(std::string_view level) {
    static const std::map<std::string_view, spdlog::level::level_enum> level_map {
        {"TRACE", spdlog::level::trace},
        {"DEBUG", spdlog::level::debug},
        {"INFO", spdlog::level::info},
        {"WARN", spdlog::level::warn},
        {"ERROR", spdlog::level::err},
        {"CRITICAL", spdlog::level::critical}
    };

    auto it = level_map.find(level);
    if (it != level_map.end()) {
        return it->second;
    }

    throw std::runtime_error("Invalid log level: " + static_cast<std::string>(level));
}

void Logger::shutdown(){
    if(m_logger){
        m_logger->flush(); // Принудительный сброс логов на диск
        spdlog::drop(m_logger->name()); // Удаляем логгер из списка
    }
    m_logger = nullptr;
}

// Базовые методы логирования
void Logger::trace(std::string_view message) {
    if (m_logger) m_logger->trace(message);
}

void Logger::debug(std::string_view message) {
    if (m_logger) m_logger->debug(message);
}

void Logger::info(std::string_view message) {
    if (m_logger) m_logger->info(message);
}

void Logger::warn(std::string_view message) {
    if (m_logger) m_logger->warn(message);
}

void Logger::error(std::string_view message) {
    if (m_logger) m_logger->error(message);
}

void Logger::critical(std::string_view message) {
    if (m_logger) m_logger->critical(message);
}

// Методы логики приложения
void Logger::session_created(std::string_view imsi) {
    if (m_logger) m_logger->trace("Session created imsi={}", imsi);
}

void Logger::session_rejected(std::string_view imsi, std::string_view reason) {
    if (m_logger) m_logger->warn("Session rejected imsi={} reason={}", imsi, reason);
}

void Logger::session_deleted(std::string_view imsi) {
    if (m_logger) m_logger->info("Session deleted imsi={}", imsi);
}

void Logger::udp_request(std::string_view imsi, std::string_view response) {
    if (m_logger) m_logger->debug("UDP request imsi={} response={}", imsi, response);
}

void Logger::http_request(std::string_view endpoint, std::string_view client_ip) {
    if (m_logger) m_logger->info("HTTP request endpoint={} client_ip={}", endpoint, client_ip);
}