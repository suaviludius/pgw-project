#ifndef I_CDR_WRITER
#define I_CDR_WRITER

#include "types.h"

class ICdrWriter {
public:
    virtual ~ICdrWriter() = default;

    virtual void writeAction(pgw::types::ConstImsi imsi, std::string_view action) = 0;
};

#endif // I_CDR_WRITER