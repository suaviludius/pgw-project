#ifndef PGW_DATABASE_CDR_WRITER_H
#define PGW_DATABASE_CDR_WRITER_H

#include "ICdrWriter.h"
#include "DatabaseManager.h"
#include "pgw/common/types.h"

#include <memory>
#include <queue>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

// Класс для записи CDR (Call Detail Records) в SQLite базу данных
// Использует batch-вставку с транзакциями и фоновый поток для flush
// Каждая запись содержит IMSI абонента, действие и временную метку

namespace pgw {

class DatabaseCdrWriter : public ICdrWriter {
    // Буфер для batch-записи
    struct CdrEntry {
        std::string imsi;
        std::string action;
    };

    // Рамзер буффера для хранения CDR записей
    static constexpr size_t BATCH_SIZE = 100;

    // Период выполнения flush в database
    static constexpr auto FLUSH_INTERVAL = std::chrono::seconds(3);

    // Кэш подготовленных запросов
    std::unordered_map<std::string, sqlite3_stmt*> m_statements;

    // Очередь CDR записей
    std::queue<CdrEntry> m_buffer;

    // Флаг для остановки m_thread
    std::atomic<bool> m_stop{false};

    // Защищает целостность данный бm_buffer при flush
    std::mutex m_mutex;

    // Поток для интервального flush в database
    std::thread m_thread;

    // Будем будить поток, когда захотим
    std::condition_variable m_cv;

    // Менеджер базы данных
    std::shared_ptr<pgw::DatabaseManager> m_db;

    // Запуск фонового выполнения flush
    void flushLoop();

public:
    explicit DatabaseCdrWriter(std::shared_ptr<pgw::DatabaseManager> db);
    ~DatabaseCdrWriter();

    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;

    std::vector<CdrRecord> getRecentRecords(size_t limit) override;

    std::optional<int> getRecordCount() override;

     // Создаёт таблицу CDR (вызывается при инициализации)
    bool createTable();

    // Принудительный сброс буфера (выполнеят накопившееся в m_buffer запросы)
    void flush();
};

} // namespace pgw

#endif // PGW_DATABASE_CDR_WRITER_H
