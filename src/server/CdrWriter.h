#ifndef CDR_WRITER_H
#define CDR_WRITER_H

#include "ICdrWriter.h" // Интерфейсный класс

#include <fstream>

class CdrWriter : public ICdrWriter {
    std::ofstream m_file;
public:
    explicit CdrWriter(pgw::types::ConstFilePath filename);
    ~CdrWriter();

    void writeAction(pgw::types::ConstImsi imsi, std::string_view action) override;
};

#endif // CDR_WRITER_H