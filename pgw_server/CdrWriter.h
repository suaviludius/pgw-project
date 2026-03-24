#ifndef PGW_CDR_WRITER_H
#define PGW_CDR_WRITER_H

#include "ICdrWriter.h"
#include "types.h"
#include "DatabaseManager.h"

#include <memory>

// Класс для записи CDR (Call Detail Records) в SQLite базу данных
// Каждая запись содержит IMSI абонента, действие и временную метку

namespace pgw {

class CdrWriter : public ICdrWriter {
    // Менеджер базы данных
    std::unique_ptr<DatabaseManager> m_db;

public:
    explicit CdrWriter(const std::string& dbPath);
    ~CdrWriter();

    // Инициализация подключения к БД
    bool initialize();

    // Записывает действие в CDR базу данных
    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;

    // Записывает событие в базу событий
    void writeEvent(std::string_view level, std::string_view message);

    // Проверка подключения
    bool isConnected() const;
};

} // namespace pgw

#endif // PGW_CDR_WRITER_H
