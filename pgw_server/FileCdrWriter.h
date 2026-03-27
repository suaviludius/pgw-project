#ifndef PGW_FILE_CDR_WRITER_H
#define PGW_FILE_CDR_WRITER_H

#include "ICdrWriter.h"
#include "types.h"

#include <fstream>

// Класс для записи CDR (Call Detail Records) в файл
// Каждая запись содержит временную метку, IMSI абонента и действие

namespace pgw {

class FileCdrWriter : public ICdrWriter {
    // Файловый поток для записи
    std::ofstream m_file;
public:
    explicit FileCdrWriter(pgw::types::constFilePath_t filename);
    ~FileCdrWriter();

    // Записывает действие в CDR файл с временной меткой
    // Формат записи: IMSI, ACTION, [TIMESTAMP]
    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;
};

} // namespace pgw

#endif // PGW_FILE_CDR_WRITER_H