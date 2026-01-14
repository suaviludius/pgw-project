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

    virtual CreateResult createSession(pgw::types::ConstImsi imsi) = 0;
    virtual bool hasSession(pgw::types::ConstImsi imsi) const = 0;
    virtual size_t getSessionCount() const = 0;
    virtual void requestGracefulShutdown() = 0;
};

#endif // I_SESSION_MANAGER_H