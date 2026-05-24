#ifndef PGW_FILE_CDR_WRITER_H
#define PGW_FILE_CDR_WRITER_H

#include "ICdrWriter.h"
<<<<<<< HEAD:src/pgw/server/FileCdrWriter.h
#include "pgw/common/types.h"
=======
#include "types.h"
>>>>>>> main:pgw_server/FileCdrWriter.h

#include <fstream>

// Класс для записи CDR (Call Detail Records) в файл
// Каждая запись содержит временную метку, IMSI абонента и действие

namespace pgw {

class FileCdrWriter : public ICdrWriter {
<<<<<<< HEAD:src/pgw/server/FileCdrWriter.h
    // Имя CDR файла
    std::string m_filename;

    // Файловый поток записи
    std::ofstream m_writeFile;

    // Файловый поток чтения
    std::ifstream m_readFile;

    // Позиции записей в файле
    std::vector<std::streampos> m_index;

    // Количество записей в файле
    size_t m_recordCount = 0;

    // Парсинг файла для чтения
    CdrRecord parseLine(const std::string& line) const;

=======
    // Файловый поток для записи
    std::ofstream m_file;
>>>>>>> main:pgw_server/FileCdrWriter.h
public:
    explicit FileCdrWriter(pgw::types::constFilePath_t filename);
    ~FileCdrWriter();

<<<<<<< HEAD:src/pgw/server/FileCdrWriter.h
    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;

    std::vector<CdrRecord> getRecentRecords(size_t limit = 100) override;

    std::optional<int> getRecordCount() override;
=======
    // Записывает действие в CDR файл с временной меткой
    // Формат записи: IMSI, ACTION, [TIMESTAMP]
    void writeAction(pgw::types::constImsi_t imsi, std::string_view action) override;
>>>>>>> main:pgw_server/FileCdrWriter.h
};

} // namespace pgw

<<<<<<< HEAD:src/pgw/server/FileCdrWriter.h
#endif // PGW_FILE_CDR_WRITER_H
=======
#endif // PGW_FILE_CDR_WRITER_H
>>>>>>> main:pgw_server/FileCdrWriter.h
