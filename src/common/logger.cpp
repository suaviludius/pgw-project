#include "logger.h"
#include <iostream> // Для использования std::cerr
#include <map>

Logger::LogPtrType Logger::m_logger = nullptr;
bool Logger::m_initialized = false;

void Logger::init(const Config& config){
    if (m_initialized) {
        return; // Уже инициализирован
    }
    try{
        // Определяем приемники логов
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(config.getLogFile());
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        // Создаем логгер
        m_logger = std::make_shared<spdlog::logger>("pgw");
        m_logger->sinks().push_back(file_sink);
        m_logger->sinks().push_back(console_sink);
        m_logger->set_level(parse_level(config.getLogLevel()));
        m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

        spdlog::set_default_logger(m_logger);

        m_initialized = true;
        info("Logger initialized successfully");
    }
    catch(const std::exception& e){
        std::cerr << "Logger initialization failed: " << e.what() << '\n';
        throw;
    }
}

Logger::LogLevelType Logger::parse_level(const std::string& level) {
    static const std::map<std::string, spdlog::level::level_enum> level_map = {
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

    throw std::runtime_error("Invalid log level: " + level);
}

// Базовые методы логирования
void Logger::trace(const std::string& message) {
    if (m_initialized) m_logger->trace(message);
}

void Logger::debug(const std::string& message) {
    if (m_initialized) m_logger->debug(message);
}

void Logger::info(const std::string& message) {
    if (m_initialized) m_logger->info(message);
}

void Logger::warn(const std::string& message) {
    if (m_initialized) m_logger->warn(message);
}

void Logger::error(const std::string& message) {
    if (m_initialized) m_logger->error(message);
}

void Logger::critical(const std::string& message) {
    if (m_initialized) m_logger->critical(message);
}

// Методы логики приложения
void Logger::session_created(const std::string& imsi) {
    if (m_initialized) m_logger->trace("Session created imsi={}", imsi);
}

void Logger::session_rejected(const std::string& imsi, const std::string& reason) {
    if (m_initialized) m_logger->warn("Session rejected imsi={} reason={}", imsi, reason);
}

void Logger::session_deleted(const std::string& imsi) {
    if (m_initialized) m_logger->info("Session deleted imsi={}", imsi);
}

void Logger::udp_request(const std::string& imsi, const std::string& response) {
    if (m_initialized) m_logger->debug("UDP request imsi={} response={}", imsi, response);
}

void Logger::http_request(const std::string& endpoint, const std::string& client_ip) {
    if (m_initialized) m_logger->info("HTTP request endpoint={} client_ip={}", endpoint, client_ip);
}