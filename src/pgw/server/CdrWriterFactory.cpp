#include "CdrWriterFactory.h"
#include "FileCdrWriter.h"
#include "DatabaseCdrWriter.h"

#include <stdexcept>

namespace pgw {

std::unique_ptr<ICdrWriter> CdrWriterFactory::createFile(pgw::types::constFilePath_t filename) {
    return std::make_unique<pgw::FileCdrWriter>(filename);
}

std::unique_ptr<ICdrWriter> CdrWriterFactory::createDatabase(std::shared_ptr<pgw::DatabaseManager> db) {
    return std::make_unique<pgw::DatabaseCdrWriter>(db);
}

} // namespace pgw
