#include "logger.h"

#include <spdlog/sinks/rotating_file_sink.h> // Для записи логов в файл
#include <spdlog/sinks/stdout_color_sinks.h> // Для цветного вывода логов в консоль

#include <iostream> // Для использования std::cerr
#include <map>

namespace pgw {

std::shared_ptr<spdlog::logger> logPtr = nullptr;

void logger::init(std::string_view logFile,
                  std::string_view logLevel,
                  size_t maxFileSize,
                  size_t maxFiles){
    if (logPtr) { // Уже инициализирован
        LOG_WARN("Logger already initialised");
        return;
    }
    try{
        // Определяем приемники логов (mt - мультипоточный)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            std::string(logFile),
            maxFileSize, // 5 MB максимальный размер файл
            maxFiles,    // Хранить 3 ротированный файл
            false        // Не ротировать при открытии
        );
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        // Создаем логгер
        logPtr = std::make_shared<spdlog::logger>("pgw");
        logPtr->sinks().push_back(file_sink);
        logPtr->sinks().push_back(console_sink);
        logPtr->set_level(parse_level(logLevel));
        logPtr->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

        // Регистрируем как логгер по умолчанию
        spdlog::register_logger(logPtr);
        spdlog::set_default_logger(logPtr);

        LOG_INFO("Logger initialized successfully with level: {}", logLevel);
    }
    catch(const std::exception& e){
        std::cerr << "Logger initialization failed: " << e.what() << '\n';
        throw;
    }
}

logger::level logger::parse_level(std::string_view level) {
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

    throw std::runtime_error("Invalid log level: " + std::string(level));
}

void logger::shutdown(){
    if(logPtr){
        logPtr->flush(); // Принудительный сброс логов на диск
        spdlog::drop(logPtr->name()); // Удаляем логгер из списка
        logPtr.reset();
    } else {
        LOG_WARN("Cannot set level: logger not initialized");
    }
}

bool logger::isInit() {
    return logPtr != nullptr;
}

void logger::set_level(spdlog::level::level_enum level) {
    if (logPtr) {
        logPtr->set_level(level);
        LOG_INFO("Log level changed to: {}", spdlog::level::to_string_view(level));
    } else {
        LOG_WARN("Cannot set level: logger not initialized");
    }
}

} // namespace pgw