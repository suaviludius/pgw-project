#ifndef I_SESSION_MANAGER_H
#define I_SESSION_MANAGER_H

#include "types.h"

class ISessionManager {
public:
    enum class CreateResult {
        CREATED,
        REJECTED_BLACKLIST,
        ALREADY_EXISTS,
        ERROR
    };

    virtual ~ISessionManager() = default;

    virtual CreateResult createSession(pgw::types::constImsi_t imsi) = 0;
    virtual bool hasSession(pgw::types::constImsi_t imsi) const = 0;
    virtual bool addToBlacklist(pgw::types::constImsi_t imsi) = 0;
    virtual size_t countActiveSession() const = 0;
};

#endif // I_SESSION_MANAGER_H