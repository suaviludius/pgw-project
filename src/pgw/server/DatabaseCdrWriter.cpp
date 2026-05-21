#include "DatabaseCdrWriter.h"

#include "logger.h"


namespace pgw {

// SQL для создания таблиц
constexpr const char* SQL_SCHEMA_CREATE = R"(
    CREATE TABLE IF NOT EXISTS cdr_records (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        imsi        TEXT(15) NOT NULL,
        action      TEXT(20) NOT NULL,
        timestamp   DATETIME DEFAULT CURRENT_TIMESTAMP
    );
    CREATE INDEX IF NOT EXISTS idx_cdr_imsi ON cdr_records(imsi);
    CREATE INDEX IF NOT EXISTS idx_cdr_timestamp ON cdr_records(timestamp);
)";

// Prepared statements
constexpr const char* SQL_WRITE_CDR = "INSERT INTO cdr_records (imsi, action) VALUES (?, ?)";
constexpr const char* SQL_RECENT_CDR = "SELECT imsi, action, timestamp FROM cdr_records ORDER BY timestamp DESC LIMIT ?";
constexpr const char* SQL_COUNT = "SELECT COUNT(*) FROM cdr_records";

DatabaseCdrWriter::DatabaseCdrWriter(std::shared_ptr<pgw::DatabaseManager> db)
    : m_db(std::move(db)){
    createTable();
    LOG_INFO("Database CDR writer initialized");
}

DatabaseCdrWriter::~DatabaseCdrWriter(){
    LOG_INFO("Database CDR writer deleted");
}

bool DatabaseCdrWriter::createTable() {
    if (!m_db || !m_db->isConnected()) return false;
    return m_db->execute(SQL_SCHEMA_CREATE);
}

void DatabaseCdrWriter::writeAction(pgw::types::constImsi_t imsi, std::string_view action){
    if (!m_db || !m_db->isConnected()) return;

    auto stmt = m_db->prepareStatement(SQL_WRITE_CDR);
    if (!stmt) return;

    sqlite3_bind_text(stmt, 1, imsi.data(), static_cast<int>(imsi.size()), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, action.data(), static_cast<int>(action.size()), SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<CdrRecord> DatabaseCdrWriter::getRecentRecords(size_t limit) {
    std::vector<CdrRecord> result;
    if (!m_db || !m_db->isConnected()) return result;

    auto stmt = m_db->prepareStatement(SQL_RECENT_CDR);
    if (!stmt) return result;

    // Определяем количество записей для чтения
    limit = std::min(limit, ICdrWriter::READ_CDR_LIMIT);

    sqlite3_bind_int(stmt, 1, static_cast<int>(limit));
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CdrRecord rec;
        rec.imsi = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        rec.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        rec.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.push_back(std::move(rec));
    }

    sqlite3_finalize(stmt);
    return result;
}

std::optional<int> DatabaseCdrWriter::getRecordCount() {
    if (!m_db || !m_db->isConnected()) return std::nullopt;

    auto val = m_db->queryValue(SQL_COUNT);

    if (!val) return std::nullopt;

    try {
        return std::stoi(*val);
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace pgw
