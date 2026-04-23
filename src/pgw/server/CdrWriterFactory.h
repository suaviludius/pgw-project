#ifndef PGW_CDR_WRITER_FACTORY_H
#define PGW_CDR_WRITER_FACTORY_H

#include "ICdrWriter.h"
#include "types.h"

#include <memory>
#include <string_view>

namespace pgw {

// Forward declaration
class DatabaseManager;

// Фабрика для создания CDR writer'ов
class CdrWriterFactory {
public:
    // Создает CDR writer для записи в файл
    static std::unique_ptr<ICdrWriter> createFile(pgw::types::constFilePath_t filename);

    // Создает CDR writer для записи в базу данных
    static std::unique_ptr<ICdrWriter> createDatabase(std::shared_ptr<pgw::DatabaseManager> db);
};

} // namespace pgw

#endif // PGW_CDR_WRITER_FACTORY_H
