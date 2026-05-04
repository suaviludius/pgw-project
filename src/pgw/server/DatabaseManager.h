#ifndef PGW_DATABASE_MANAGER_H
#define PGW_DATABASE_MANAGER_H

#include "IDatabaseManager.h"

#include <sqlite3.h>


// Менеджер базы данных SQLite

namespace pgw {

class DatabaseManager : public IDatabaseManager{
private:
    // Структура для интерпритации данных БД в виде объетка SQLite
    sqlite3* m_db = nullptr;

    // Путь к файлу с БД
    std::string m_dbPath;

    // Prepared statements для быстрой вставки
    sqlite3_stmt* m_stmtWriteCdr = nullptr;

    // Подготовка statement для запроса
    sqlite3_stmt* prepareStatement(const std::string& sql);

    // Очистка ресурсов
    void cleanup();

public:
    explicit DatabaseManager(const std::string& dbPath);
    ~DatabaseManager();

    // Запрещаем копирование (SQLite connection не потокобезопасен при копировании)
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool isConnected() const override { return m_db != nullptr; }

    bool writeCdr(std::string_view imsi, std::string_view action) override;

    std::vector<CdrRecord> getRecentCdr(size_t limit = READ_CDR_LIMIT) override;

    std::optional<int> count(const std::string& table) override;

    // Инициализация БД (создание таблиц)
    bool initialize();

    // Выполнение SQL без возврата результата
    bool execute(const std::string& sql);

    //Выполнение SQL с возвратом одного значения
    std::optional<std::string> queryValue(const std::string& sql);

};

} // namespace pgw

#endif // PGW_DATABASE_MANAGER_H
