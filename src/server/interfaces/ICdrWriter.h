#ifndef I_CDR_WRITER
#define I_CDR_WRITER

#include "types.h"

class ICdrWriter {
public:
    static constexpr std::string_view SESSION_CREATED = "SESSION_CREATED";
    static constexpr std::string_view SESSION_DELETED = "SESSION_DELETED";
    static constexpr std::string_view SESSION_REJECTED = "SESSION_REJECTED";
    static constexpr std::string_view SESSION_ERROR = "SESSION_ERROR";

    virtual ~ICdrWriter() = default;

    virtual void writeAction(pgw::types::ConstImsi imsi, std::string_view action) = 0;
};

#endif // I_CDR_WRITER