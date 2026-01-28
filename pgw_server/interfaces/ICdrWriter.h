#ifndef I_CDR_WRITER
#define I_CDR_WRITER

#include "types.h"

class ICdrWriter {
public:
    static constexpr std::string_view SESSION_CREATED = "CREATED";
    static constexpr std::string_view SESSION_DELETED = "DELETED";
    static constexpr std::string_view SESSION_REJECTED = "REJECTED";
    static constexpr std::string_view SESSION_ERROR = "ERROR";

    virtual ~ICdrWriter() = default;

    virtual void writeAction(pgw::types::constImsi_t imsi, std::string_view action) = 0;
};

#endif // I_CDR_WRITER