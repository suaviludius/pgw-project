#ifndef PGW_CDR_WRITER_H
#define PGW_CDR_WRITER_H

#include "ICdrWriter.h"
#include "types.h"

#include <fstream>

// Класс для записи CDR (Call Detail Records) в файл
// Каждая запись содержит временную метку, IMSI абонента и действие

class CdrWriter : public ICdrWriter {
    // Файловый поток для записи
    std::ofstream m_file;
public:
    explicit CdrWriter(pgw::types::constFilePath_t filename);
    ~CdrWriter();

    // Записывает действие в CDR файл с временной меткой
    // Формат записи: IMSI, ACTION, [TIMESTAMP]
    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;
};

#endif // PGW_CDR_WRITER_H