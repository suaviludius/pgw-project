#include "CdrWriter.h"

CdrWriter::CdrWriter(pgw::types::ConstFilePath filename){
    // К сожалению, для open() строка должна содержать '\0' символ
    m_file.open(static_cast<std::string>(filename), std::ios::app);
    if(m_file.is_open()){
        m_file << "CDR strated\n";
        m_file << "IMSI, Action, Timestamp\n";
    }
}

CdrWriter::~CdrWriter(){
    if(m_file.is_open()){
        m_file << "CDR ended\n";
        m_file.close();
    }
}

void CdrWriter::writeAction(pgw::types::ConstImsi imsi, std::string_view action){
    if (!m_file.is_open()) return;

    auto now {std::chrono::system_clock::now()}; // формат для вемени
    auto time {std::chrono::system_clock::to_time_t(now)}; // формат для форматирования времени

    m_file  << imsi << ", "
            << action << ", "
            << std::asctime(std::localtime(&time)); // Преобразует структуру локального врмени в строку

    m_file.flush();
}