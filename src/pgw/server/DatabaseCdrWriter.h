#ifndef PGW_DATABASE_CDR_WRITER_H
#define PGW_DATABASE_CDR_WRITER_H

#include "ICdrWriter.h"
#include "types.h"
#include "DatabaseManager.h"

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

    // Записывает действие в CDR базу данных
    // Формат записи: IMSI, ACTION, sTIMESTAMP
    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;
};

} // namespace pgw

#endif // PGW_DATABASE_CDR_WRITER_H
