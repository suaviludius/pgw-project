#ifndef PGW_DATABASE_MANAGER_H
#define PGW_DATABASE_MANAGER_H

#include "IDatabaseManager.h"

#include <sqlite3.h>
#include <optional>
#include <string>


// Менеджер базы данных SQLite

namespace pgw {

class DatabaseManager : public IDatabaseManager{
private:
    // Структура для интерпритации данных БД в виде объетка SQLite
    sqlite3* m_db = nullptr;

    // Путь к файлу с БД
    std::string m_dbPath;

    // Очистка ресурсов
    void cleanup();

public:
    explicit DatabaseManager(const std::string& dbPath);
    ~DatabaseManager();

    // Запрещаем копирование (SQLite connection не потокобезопасен при копировании)
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool isConnected() const override { return m_db != nullptr; }

    bool initialize() override;

    bool execute(const std::string& sql) override;

    // Подготовка statement для запроса
    sqlite3_stmt* prepareStatement(const std::string& sql);

    //Выполнение SQL с возвратом одного значения
    std::optional<std::string> queryValue(const std::string& sql);
};

} // namespace pgw

#endif // PGW_DATABASE_MANAGER_H
