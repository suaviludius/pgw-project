#ifndef PGW_DATABASE_CDR_WRITER_H
#define PGW_DATABASE_CDR_WRITER_H

#include "ICdrWriter.h"
#include "DatabaseManager.h"
#include "pgw/common/types.h"

#include <memory>

// Класс для записи CDR (Call Detail Records) в SQLite базу данных
// Каждая запись содержит IMSI абонента, действие и временную метку

namespace pgw {

class DatabaseCdrWriter : public ICdrWriter {
    // Менеджер базы данных (ссылка, владение вне этого класса)
    std::shared_ptr<pgw::DatabaseManager> m_db;

public:
    explicit DatabaseCdrWriter(std::shared_ptr<pgw::DatabaseManager> db);
    ~DatabaseCdrWriter();

    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;

    std::vector<CdrRecord> getRecentRecords(size_t limit) override;

    std::optional<int> getRecordCount() override;

    // Создаёт таблицу CDR (вызывается при инициализации)
    bool createTable();
};

} // namespace pgw

#endif // PGW_DATABASE_CDR_WRITER_H
