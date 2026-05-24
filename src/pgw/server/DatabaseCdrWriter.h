#ifndef PGW_DATABASE_CDR_WRITER_H
#define PGW_DATABASE_CDR_WRITER_H

#include "ICdrWriter.h"
<<<<<<< HEAD:src/pgw/server/DatabaseCdrWriter.h
#include "DatabaseManager.h"
#include "pgw/common/types.h"
=======
#include "types.h"
#include "DatabaseManager.h"
>>>>>>> main:pgw_server/DatabaseCdrWriter.h

#include <memory>

// Класс для записи CDR (Call Detail Records) в SQLite базу данных
// Каждая запись содержит IMSI абонента, действие и временную метку

namespace pgw {

class DatabaseCdrWriter : public ICdrWriter {
    // Менеджер базы данных (ссылка, владение вне этого класса)
    std::shared_ptr<pgw::DatabaseManager> m_db;
<<<<<<< HEAD:src/pgw/server/DatabaseCdrWriter.h

=======
>>>>>>> main:pgw_server/DatabaseCdrWriter.h
public:
    explicit DatabaseCdrWriter(std::shared_ptr<pgw::DatabaseManager> db);
    ~DatabaseCdrWriter();

<<<<<<< HEAD:src/pgw/server/DatabaseCdrWriter.h
    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;

    std::vector<CdrRecord> getRecentRecords(size_t limit) override;

    std::optional<int> getRecordCount() override;

    // Создаёт таблицу CDR (вызывается при инициализации)
    bool createTable();
=======
    // Записывает действие в CDR базу данных
    // Формат записи: IMSI, ACTION, sTIMESTAMP
    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;
>>>>>>> main:pgw_server/DatabaseCdrWriter.h
};

} // namespace pgw

#endif // PGW_DATABASE_CDR_WRITER_H
