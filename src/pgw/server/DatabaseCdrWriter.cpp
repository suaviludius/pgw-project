#include "DatabaseCdrWriter.h"

#include "pgw/common/logger.h"

#include <stdexcept>


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
constexpr const char* SQL_BEGIN_IMMEDIATE = "BEGIN IMMEDIATE";
constexpr const char* SQL_ROLLBACK = "ROLLBACK";
constexpr const char* SQL_COMMIT = "COMMIT";


DatabaseCdrWriter::DatabaseCdrWriter(std::shared_ptr<DatabaseManager> db)
    : m_db(std::move(db)){
    if (!m_db || !m_db->isConnected()){
        throw std::runtime_error("Failed connect database for CDR writer");
    }
    if (!createTable()) {
        throw std::runtime_error("Failed to create CDR table");
    }
    m_statements[SQL_WRITE_CDR] = m_db->prepareStatement(SQL_WRITE_CDR);
    m_statements[SQL_RECENT_CDR] = m_db->prepareStatement(SQL_RECENT_CDR);
    if (m_statements[SQL_WRITE_CDR] == nullptr || m_statements[SQL_RECENT_CDR] == nullptr) {
        throw std::runtime_error("Failed to prepare statements");
    }

    m_thread = std::thread(&DatabaseCdrWriter::flushLoop, this);
    LOG_INFO("Database CDR writer started (batch={}, interval={}s)", BATCH_SIZE, FLUSH_INTERVAL.count());
}

DatabaseCdrWriter::~DatabaseCdrWriter(){
    m_stop = true;
    m_cv.notify_one();
    if (m_thread.joinable()){
        m_thread.join();
    }
    // Последний сброс
    flush();  

    // Освобождаем prepared statements (без этого БД может не закрыть все соединения)
    for (auto& [sql, stmt] : m_statements) {
        if (stmt) {
            sqlite3_finalize(stmt);
        }
    }
    m_statements.clear();

    LOG_INFO("Database CDR writer deleted");
}

bool DatabaseCdrWriter::createTable() {
    return m_db->execute(SQL_SCHEMA_CREATE);
}

void DatabaseCdrWriter::writeAction(pgw::types::constImsi_t imsi, std::string_view action){
    if (!m_db || !m_db->isConnected()) {
        LOG_WARN("Database not connected, cannot write CDR");
        return;
    }

    { // Добавляем запись в буфер (с защитой от гонки данных)
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.push({std::string(imsi), std::string(action)});
        // Если буфер переполнился -будим поток для немедленного сброса
        if (m_buffer.size() >= BATCH_SIZE)
            m_cv.notify_one();
    }

    LOG_DEBUG("CDR buffered: {} - {} (buffer size: {})", imsi, action, m_buffer.size());
}

void DatabaseCdrWriter::flush(){
    std::queue<CdrEntry> toFlush;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_buffer.empty()) return;
        // Меняемся указателями O(1): m_buffer <-> toFlush
        toFlush.swap(m_buffer);
    }

    // Начало транзакции (блокирует для записи)
    if (!m_db->execute(SQL_BEGIN_IMMEDIATE)) {
        LOG_ERROR("Failed to begin transaction");
        // Возвращаем записи в буффер
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!toFlush.empty()) toFlush.swap(m_buffer);
        return;
    }

    // Вставка всех записей из буфера
    size_t inserted = 0;
    while (!toFlush.empty()) {
        const auto& entry = toFlush.front();
        const auto& stmt = m_statements[SQL_WRITE_CDR];

        sqlite3_bind_text(stmt, 1, entry.imsi.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, entry.action.c_str(), -1, SQLITE_STATIC);

        int rc = sqlite3_step(stmt);

        // Сбрасываем ДЛЯ СЛЕДУЮЩЕЙ ЗАПИСИ
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        if (rc != SQLITE_DONE) {
            m_db->execute(SQL_ROLLBACK);
            LOG_ERROR("Failed to insert CDR record");
            // Возвращаем оставшиеся записи
            std::lock_guard<std::mutex> lock(m_mutex);
            while (!toFlush.empty()) {
                m_buffer.push(toFlush.front());
                toFlush.pop();
            }
            return;
        }

        toFlush.pop();
        ++inserted;
    }

    // Окончание транзакции
    if (!m_db->execute(SQL_COMMIT)) {
        LOG_ERROR("Failed to commit transaction");
        return;
    }

    LOG_DEBUG("Flushed {} CDR records", inserted);

}

void DatabaseCdrWriter::flushLoop(){
    while (!m_stop) {
        std::unique_lock<std::mutex> lock(m_mutex);
        // Ждём: stop или буфер не пуст или разбудимся по таймауту
        m_cv.wait_for(lock, FLUSH_INTERVAL, [this] {
            return m_stop || m_buffer.size() >= BATCH_SIZE;
        });
        lock.unlock();

        if (m_stop) break;

        flush();
    }
}

// TODO: сделать буффер для хранения limit записей что считываются из БД с каким-то интервалом,
// это уменьшит обращения к БД (обновлять буффер по таймеру в другом потоке)
std::vector<CdrRecord> DatabaseCdrWriter::getRecentRecords(size_t limit) {
    std::vector<CdrRecord> result;
    if (!m_db || !m_db->isConnected()) return result;

    // Flush буфера перед чтением (думаю для HMI это не обязательно)
    flush();

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

    // Flush буфера перед чтением (думаю для HMI это не обязательно)
    // flush();

    auto val = m_db->queryValue(SQL_COUNT);

    if (!val) return std::nullopt;

    try {
        return std::stoi(*val);
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace pgw
