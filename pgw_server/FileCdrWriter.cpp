#include "FileCdrWriter.h"

#include "logger.h"

namespace pgw {

FileCdrWriter::FileCdrWriter(pgw::types::constFilePath_t filename){
    // К сожалению, для open() строка должна содержать '\0' символ
    m_file.open(static_cast<std::string>(filename), std::ios::app);
    if(m_file.is_open()){
        m_file << "CDR strated\n";
        m_file << "IMSI, Action, Timestamp\n";
    }
    LOG_INFO("CDR writer initialized");
}

FileCdrWriter::~FileCdrWriter(){
    if(m_file.is_open()){
        m_file << "CDR ended\n";
        m_file.close();
    }
    LOG_INFO("CDR writer deleted");
}

void FileCdrWriter::writeAction(pgw::types::constImsi_t imsi, std::string_view action){
    if (!m_file.is_open()) return;

    auto now {std::chrono::system_clock::now()}; // формат для вемени
    auto time {std::chrono::system_clock::to_time_t(now)}; // формат для форматирования времени

    m_file  << imsi << ", "
            << action << ", "
            << std::asctime(std::localtime(&time)); // Преобразует структуру локального врмени в строку

    m_file.flush();
}

} // namespace pgw
