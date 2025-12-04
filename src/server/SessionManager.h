#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "config.h"
#include "Session.h"

class SessionManager {
public:
    enum class CreateResult {
        CREATED,
        REJECTED_BLACKLIST,
        ALREADY_EXISTS,
        ERROR
    };

private:
    pgw::types::Container<pgw::types::ConstImsi> m_blacklist;
    pgw::types::Container<std::unique_ptr<Session>> m_sessions; // RAII, поэтому используем умные указатели
    pgw::types::Seconds m_sessionTimeoutSec;
    pgw::types::Rate m_shutdownRate;

    auto findSession(pgw::types::ConstImsi imsi); // Для внутренних методов auto можно использовать
public:
    explicit SessionManager(const Config& config);
    CreateResult createSession(pgw::types::ConstImsi imsi);
    void removeSession(pgw::types::ConstImsi imsi);
    void gracefulShutdown(pgw::types::Rate rate);
    void cleanTimeoutSessions();

    bool hasSession(pgw::types::ConstImsi imsi) const {return (findSession(imsi) != m_sessions.end());}
    size_t getSessionCount() const {return m_sessions.size();}
};

#endif // SESSION_MANAGER_H