#ifndef CDR_WRITER_H
#define CDR_WRITER_H

#include "types.h"

#include <fstream>

class CdrWriter {
    std::ofstream m_file;
public:
    explicit CdrWriter(pgw::types::ConstFilePath filename);
    ~CdrWriter();
    void writeAction(pgw::types::ConstImsi imsi, std::string_view action);
};

#endif // CDR_WRITER_H