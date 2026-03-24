#ifndef PGW_DATABASE_MANAGER_H
#define PGW_DATABASE_MANAGER_H

#include <sqlite3.h>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>

namespace pgw {

// CDR запись для возврата из БД
struct CdrRecord {
    std::string imsi;
    std::string action;
    std::string timestamp;
};

// Событие для возврата из БД
struct EventRecord {
    std::string level;
    std::string message;
    std::string timestamp;
};


// Менеджер базы данных SQLite
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& dbPath);
    ~DatabaseManager();

    // Запрещаем копирование (SQLite connection не потокобезопасен при копировании)
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // Инициализация БД (создание таблиц)
    bool initialize();

    // Проверка подключения
    bool isConnected() const { return m_db != nullptr; }

    // Запись CDR записи
    bool writeCdr(std::string_view imsi, std::string_view action);

    // Запись события
    bool writeEvent(std::string_view level, std::string_view message);

    // Получение последних CDR записей
    std::vector<CdrRecord> getRecentCdr(size_t limit = 100);

    //Получение последних событий
    std::vector<EventRecord> getRecentEvents(size_t limit = 100);

    // Получение количества записей в таблице
    std::optional<int> count(const std::string& table);

    // Выполнение SQL без возврата результата
    bool execute(const std::string& sql);

    //Выполнение SQL с возвратом одного значения
    std::optional<std::string> queryValue(const std::string& sql);

private:
    sqlite3* m_db = nullptr;
    std::string m_dbPath;

    // Prepared statements для быстрой вставки
    sqlite3_stmt* m_stmtWriteCdr = nullptr;
    sqlite3_stmt* m_stmtWriteEvent = nullptr;

    // Подготовка statement для запроса
    sqlite3_stmt* prepareStatement(const std::string& sql);

    // Очистка ресурсов
    void cleanup();
};

} // namespace pgw

#endif // PGW_DATABASE_MANAGER_H
