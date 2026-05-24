#include "DatabaseManager.h"
#include <stdexcept>
#include <array>

namespace pgw {

// SQL для создания таблиц
constexpr const char* SCHEMA_SQL = R"(
    CREATE TABLE IF NOT EXISTS cdr_records (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        imsi        TEXT(15) NOT NULL,
        action      TEXT(20) NOT NULL,
        timestamp   DATETIME DEFAULT CURRENT_TIMESTAMP
    );
    CREATE INDEX IF NOT EXISTS idx_cdr_imsi ON cdr_records(imsi);
    CREATE INDEX IF NOT EXISTS idx_cdr_timestamp ON cdr_records(timestamp);

    CREATE TABLE IF NOT EXISTS events (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        level       TEXT(10) NOT NULL,
        message     TEXT(500) NOT NULL,
        timestamp   TEXT(30) NOT NULL
    );
    CREATE INDEX IF NOT EXISTS idx_events_timestamp ON events(timestamp);
)";

// Prepared statements
constexpr const char* SQL_WRITE_CDR = "INSERT INTO cdr_records (imsi, action) VALUES (?, ?)";
constexpr const char* SQL_WRITE_LOG = "INSERT INTO events (level, message, timestamp) VALUES (?, ?, ?)";
constexpr const char* SQL_RECENT_CDR = "SELECT imsi, action, timestamp FROM cdr_records ORDER BY timestamp DESC LIMIT ?";
constexpr const char* SQL_RECENT_EVENTS = "SELECT level, message, timestamp FROM events ORDER BY timestamp DESC LIMIT ?";
constexpr const char* SQL_COUNT = "SELECT COUNT(*) FROM ";

DatabaseManager::DatabaseManager(const std::string& dbPath)
    : m_dbPath(dbPath)
{
}

DatabaseManager::~DatabaseManager() {
    cleanup();
}

void DatabaseManager::cleanup() {
    if (m_stmtWriteCdr) {
        sqlite3_finalize(m_stmtWriteCdr);
        m_stmtWriteCdr = nullptr;
    }
    if (m_stmtWriteLog) {
        sqlite3_finalize(m_stmtWriteLog);
        m_stmtWriteLog = nullptr;
    }
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

// TODO: стоит сделать метод добавления новых таблиц в БД
// создать вектор или еще какой тип с дискрипторами, но как это сделать
// известно только создателю SQlite и моей бабушке

bool DatabaseManager::initialize() {
    // Открываем или создаем БД
    int rc = sqlite3_open(m_dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        return false;
    }

    // Создаем таблицы
    char* errMsg = nullptr;
    rc = sqlite3_exec(m_db, SCHEMA_SQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) {
            sqlite3_free(errMsg);
        }
        return false;
    }

    // Подготавливаем statements для быстрой вставки
    m_stmtWriteCdr = prepareStatement(SQL_WRITE_CDR);
    m_stmtWriteLog = prepareStatement(SQL_WRITE_LOG);

    return m_stmtWriteCdr != nullptr && m_stmtWriteLog != nullptr;
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

// TODO: стоит избавиться от дублирования кода в writeCdr() и writeLog()
// Можно вынести реализацию в один файл и регулировать направление вывода,
// передавая дополнительный параметр в виде целевой таблицы

bool DatabaseManager::writeCdr(std::string_view imsi, std::string_view action) {
    if (!m_stmtWriteCdr) return false;

    // Сбрасываем предыдущие bindings
    sqlite3_reset(m_stmtWriteCdr);

    // Bind параметров
    sqlite3_bind_text(m_stmtWriteCdr, 1, imsi.data(), static_cast<int>(imsi.size()), SQLITE_STATIC);
    sqlite3_bind_text(m_stmtWriteCdr, 2, action.data(), static_cast<int>(action.size()), SQLITE_STATIC);

    // Выполняем
    int rc = sqlite3_step(m_stmtWriteCdr);
    return rc == SQLITE_DONE;
}

bool DatabaseManager::writeLog(std::string_view level, std::string_view message, std::string_view timestamp) {
    if (!m_stmtWriteLog) return false;

    sqlite3_reset(m_stmtWriteLog);

    sqlite3_bind_text(m_stmtWriteLog, 1, level.data(), static_cast<int>(level.size()), SQLITE_STATIC);
    sqlite3_bind_text(m_stmtWriteLog, 2, message.data(), static_cast<int>(message.size()), SQLITE_STATIC);
    sqlite3_bind_text(m_stmtWriteLog, 3, timestamp.data(), static_cast<int>(timestamp.size()), SQLITE_STATIC);

    int rc = sqlite3_step(m_stmtWriteLog);
    return rc == SQLITE_DONE;
}

std::vector<CdrRecord> DatabaseManager::getRecentCdr(size_t limit) {
    std::vector<CdrRecord> result;

    if (!m_db) return result;

    sqlite3_stmt* stmt = prepareStatement(SQL_RECENT_CDR);
    if (!stmt) return result;

    sqlite3_bind_int(stmt, 1, static_cast<int>(limit));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CdrRecord record;
        record.imsi = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.push_back(std::move(record));
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<EventRecord> DatabaseManager::getRecentEvents(size_t limit) {
    std::vector<EventRecord> result;

    if (!m_db) return result;

    sqlite3_stmt* stmt = prepareStatement(SQL_RECENT_EVENTS);
    if (!stmt) return result;

    sqlite3_bind_int(stmt, 1, static_cast<int>(limit));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EventRecord record;
        record.level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.push_back(std::move(record));
    }

    sqlite3_finalize(stmt);
    return result;
}

std::optional<int> DatabaseManager::count(const std::string& table) {
    auto value = queryValue(SQL_COUNT + table);
    if (!value) return std::nullopt;

    try {
        return std::stoi(*value);
    } catch (...) {
        return std::nullopt;
    }
}

bool DatabaseManager::execute(const std::string& sql) {
    if (!m_db) return false;

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

std::optional<std::string> DatabaseManager::queryValue(const std::string& sql) {
    if (!m_db) return std::nullopt;

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) return std::nullopt;

    std::optional<std::string> result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text) {
            result = std::string(text);
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

} // namespace pgw
