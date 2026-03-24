#include "CdrWriter.h"

#include "logger.h"

namespace pgw {

CdrWriter::CdrWriter(const std::string& dbPath)
    : m_db(std::make_unique<DatabaseManager>(dbPath))
{
    LOG_INFO("CDR writer initialized with database: {}", dbPath);
}

CdrWriter::~CdrWriter(){
    LOG_INFO("CDR writer deleted");
}

bool CdrWriter::initialize() {
    if (m_db->initialize()) {
        m_db->writeEvent("INFO", "CDR writer initialized");
        return true;
    }
    LOG_ERROR("Failed to initialize CDR writer");
    return false;
}

void CdrWriter::writeAction(pgw::types::constImsi_t imsi, std::string_view action){
    if (!m_db->isConnected()) return;

    m_db->writeCdr(imsi, action);
}

void CdrWriter::writeEvent(std::string_view level, std::string_view message) {
    if (!m_db->isConnected()) return;

    m_db->writeEvent(level, message);
}

bool CdrWriter::isConnected() const {
    return m_db->isConnected();
}

} // namespace pgw
