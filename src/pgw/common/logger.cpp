#include "pgw/common/logger.h"

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h> // Для записи логов в файл
#include <spdlog/sinks/stdout_color_sinks.h> // Для цветного вывода логов в консоль

#include <iostream> // Для использования std::cerr
#include <map>

namespace pgw {

std::shared_ptr<spdlog::logger> g_logger = nullptr;

void logger::init(std::string_view logLevel){
    if (g_logger) { // Уже инициализирован
        LOG_WARN("Logger already initialised");
        return;
    }
    try{
        // очередь на QUEUE_SIZE сообщений, 1 поток
        spdlog::init_thread_pool(QUEUE_SIZE, 1);

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        // Создаем логгер
        g_logger = std::make_shared<spdlog::async_logger>(
            "pgw",
            console_sink,
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );
        //g_logger->sinks().push_back(console_sink);
        g_logger->set_level(parse_level(logLevel));
        g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

        g_logger->flush_on(spdlog::level::warn);

        // Регистрируем как логгер по умолчанию
        spdlog::register_logger(g_logger);
        spdlog::set_default_logger(g_logger);

        LOG_INFO("Logger initialized successfully with level: {}", logLevel);
    }
    catch(const std::exception& e){
        std::cerr << "Logger initialization failed: " << e.what() << '\n';
        throw;
    }
}


void logger::addFileSink(const std::string& logFile) {
    if (!g_logger) { // Не инициализирован
        LOG_WARN("Logger not initialised");
        return;
    }

    // Определяем приемники логов (mt - мультипоточный)
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        std::string(logFile),
        FILE_SIZE,      // Максимальный размер файл
        FILES_COUNT,    // Хранить ротированных файлов
        false           // Не ротировать при открытии
    );
    file_sink->set_pattern(PATTERN);

    // Проверяем есть ли файл синк уже
    for (const auto& sink : g_logger->sinks()) {
        if (std::dynamic_pointer_cast<spdlog::sinks::rotating_file_sink_mt>(sink)) {
            LOG_DEBUG("File sink already exists, skipping");
            return;
        }
    }

    // Отключаем автоматический flush (слишком частые системные вызовы)
    g_logger->flush_on(spdlog::level::off);

    // Фоновый поток будет сбрасывать логгеры каждые FLUSH_TIMEOUT секунд
    spdlog::flush_every(std::chrono::seconds(FLUSH_TIMEOUT));

    g_logger->sinks().push_back(file_sink);

    // Логируем факт добавления (через сам логгер)
    LOG_INFO("File sink added: {}", logFile);
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
    if(g_logger){
        g_logger->flush(); // Принудительный сброс логов на диск
        spdlog::drop(g_logger->name()); // Удаляем логгер из списка
        g_logger.reset();
    } else {
        LOG_WARN("Cannot set level: logger not initialized");
    }
}

bool logger::isInit() {
    return g_logger != nullptr;
}

void logger::set_level(spdlog::level::level_enum level) {
    if (g_logger) {
        g_logger->set_level(level);
        LOG_INFO("Log level changed to: {}", spdlog::level::to_string_view(level));
    } else {
        LOG_WARN("Cannot set level: logger not initialized");
    }
}

} // namespace pgw