#include "DatabaseManager.h"

#include <stdexcept>
#include <array>

namespace pgw {

DatabaseManager::DatabaseManager(const std::string& dbPath)
    : m_dbPath(dbPath){
}

DatabaseManager::~DatabaseManager() {
    cleanup();
}

void DatabaseManager::cleanup() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool DatabaseManager::initialize() {
    // Открываем или создаем БД
    int rc = sqlite3_open(m_dbPath.c_str(), &m_db);

    if (rc != SQLITE_OK) {
        m_db = nullptr;
        return false;
    }

    // Улучшаем производительность
    // (если для усорения нужна пара строк, то газ! - усорение ~ 3x)
    execute("PRAGMA journal_mode=WAL;");           // Пишем в отдельные файлы перед БД
    execute("PRAGMA synchronous=NORMAL;");         // Быстрее, чем OFF (если ОС вылетит, то данные минус)
    execute("PRAGMA mmap_size=268435456;");        // 256MB mmap выделяем виртуальную память для БД (меньше системных вызовов)
    execute("PRAGMA cache_size=10000;");           // 10MB кэш ускорит чтение
    return true;
}

sqlite3_stmt* DatabaseManager::prepareStatement(const std::string& sql) {
    if (!m_db) return nullptr;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return nullptr;
    }

    return stmt;
}


std::optional<std::string> DatabaseManager::queryValue(const std::string& sql) {
    if (!m_db) {
        return nullptr;
    }

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) {
        return nullptr;
    }

    std::optional<std::string> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text) result = std::string(text);
    }

    sqlite3_finalize(stmt);
    return result;
}


bool DatabaseManager::execute(const std::string& sql) {
    if (!m_db) {
        return false;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        if (errMsg) {
            sqlite3_free(errMsg);
        }
        return false;
    }

    return true;
}


} // namespace pgw
