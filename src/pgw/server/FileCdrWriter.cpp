#include "FileCdrWriter.h"

#include "pgw/common/logger.h"

#include <sstream>

namespace pgw {

FileCdrWriter::FileCdrWriter(pgw::types::constFilePath_t filename)
    : m_filename(filename){

    // Открываем для записи (с добавлением в конец)
    m_writeFile.open(m_filename, std::ios::out | std::ios::app);
    if (!m_writeFile.is_open()) {
        LOG_ERROR("Cannot open CDR file for writing: {}", m_filename);
        return;
    }

    // Открываем для чтения (для индексации существующих записей)
    m_readFile.open(m_filename, std::ios::in);
    if (!m_readFile.is_open()) {
        LOG_ERROR("Cannot open CDR file for reading: {}", m_filename);
        return;
    }

    // Заносим сообзение и заголовки
    m_writeFile << "CDR started\n";
    m_writeFile << "IMSI, Action, Timestamp\n";
    m_writeFile.flush();

    LOG_INFO("File CDR writer initialized");
}

FileCdrWriter::~FileCdrWriter(){
    if(m_writeFile.is_open()){
        m_writeFile << "CDR ended\n";
        m_writeFile.close();
    }
    if(m_readFile.is_open()){
        m_readFile.close();
    }
    LOG_INFO("File CDR writer deleted");
}

void FileCdrWriter::writeAction(pgw::types::constImsi_t imsi, std::string_view action){
    if (!m_writeFile.is_open()) {
        LOG_WARN("Cannot open CDR file for writing: {}", m_filename);
        return;
    }

    // Запоминаем позицию перед записью
    std::streampos pos = m_writeFile.tellp();

    auto now {std::chrono::system_clock::now()}; // формат для вемени
    auto time {std::chrono::system_clock::to_time_t(now)}; // формат для форматирования времени

    m_writeFile << imsi << ", "
                << action << ", "
                << std::asctime(std::localtime(&time)); // Преобразует структуру локального врмени в строку

    // будем флушить в getRecentRecords()
    //m_writeFile.flush();

    // Обновляем позиции
    m_index.push_back(pos);
    m_recordCount++;

    LOG_DEBUG("CDR written: {} - {}", imsi, action);
}

std::vector<CdrRecord> FileCdrWriter::getRecentRecords(size_t limit) {
    std::vector<CdrRecord> records;

    // если буфер пуст, flush сделает пару сравнений и всё !
    // TODO: если делать HMI, то для скорости нужно сделать флаг записи с последнего flush
    if (m_writeFile.is_open()) {
        m_writeFile.flush();
    }

    if (!m_readFile.is_open()) {
        LOG_WARN("Cannot open CDR file for reading: {}", m_filename);
        return records;
    }

    // Заранее определяем количество записей для чтения
    limit = std::min(limit, ICdrWriter::READ_CDR_LIMIT);
    limit = std::min(limit, m_recordCount);
    records.reserve(limit);

    // Идем с конца индекса
    for (size_t i = m_recordCount - limit; i < m_recordCount; ++i) {
        m_readFile.clear();  // Сбрасываем флаги, один раз уже попался в эту западню
        m_readFile.seekg(m_index[i]); // Берем позицию конкретной строки

        std::string line;
        std::getline(m_readFile, line);
        records.push_back(parseLine(line));
    }

    return records;
}

std::optional<int> FileCdrWriter::getRecordCount() {
    return static_cast<int>(m_recordCount);
}

CdrRecord FileCdrWriter::parseLine(const std::string& line) const {
    CdrRecord record;
    std::stringstream ss(line);
    std::string part;
    int partIndex = 0;

    while (std::getline(ss, part, ',') && partIndex < 3) {
        // Убираем пробелы
        part.erase(0, part.find_first_not_of(" \t"));
        part.erase(part.find_last_not_of(" \t") + 1);

        switch (partIndex) {
            case 0: record.imsi = part; break;
            case 1: record.action = part; break;
            case 2: record.timestamp = part; break;
        }
        partIndex++;
    }

    return record;
}

} // namespace pgw
