#include "DatabaseCdrWriter.h"

#include "logger.h"

namespace pgw {

DatabaseCdrWriter::DatabaseCdrWriter(std::shared_ptr<pgw::DatabaseManager> db)
    : m_db(std::move(db))
{
    LOG_INFO("CDR writer initialized");
}

DatabaseCdrWriter::~DatabaseCdrWriter(){
    LOG_INFO("CDR writer deleted");
}

void DatabaseCdrWriter::writeAction(pgw::types::constImsi_t imsi, std::string_view action){
    if (!m_db->isConnected()) return;

    m_db->writeCdr(imsi, action);
}

} // namespace pgw
