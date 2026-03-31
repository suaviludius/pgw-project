#ifndef DATABASE_SINK_H
#define DATABASE_SINK_H

#include "DatabaseManager.h"

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/chrono.h>
#include <map>

// Функция преобразования уровня логирования в строку
std::string_view levelToString(spdlog::level::level_enum level);

class DatabaseSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit DatabaseSink(std::shared_ptr<pgw::DatabaseManager> dbManager)
        : m_dbManager(std::move(dbManager))
    {
        // . . .
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!m_dbManager) {
            return;
        }

        // Форматируем без лишних временных объектов
        auto timeSinceEpoch = msg.time.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeSinceEpoch);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceEpoch - seconds);

        // Используем string_view для уменьшения пупочек и лупочек
        std::string_view message(msg.payload.data(), msg.payload.size());
        std::string_view level = levelToString(msg.level);

        // Выполняем запись в БД
        m_dbManager->writeLog(
            level,
            message,
            fmt::format("{:%Y-%m-%d %H:%M:%S}.{:03d}",
                    fmt::localtime(seconds.count()),
                    milliseconds.count() % 1000)
        );
    }

    void flush_() override {
        // Ничего не делаем, тупо чилим
    }

private:
    std::shared_ptr<pgw::DatabaseManager> m_dbManager;
};

std::string_view levelToString(spdlog::level::level_enum level) {
    static const std::map<spdlog::level::level_enum, std::string_view> level_map {
        {spdlog::level::trace, "TRACE"},
        {spdlog::level::debug, "DEBUG"},
        {spdlog::level::info, "INFO"},
        {spdlog::level::warn, "WARN"},
        {spdlog::level::err, "ERROR"},
        {spdlog::level::critical, "CRITICAL"}
    };

    auto it = level_map.find(level);
    if (it != level_map.end()) {
        return it->second;
    }
    return "UNKNOWN";
}

#endif // DATABASE_SINK_H