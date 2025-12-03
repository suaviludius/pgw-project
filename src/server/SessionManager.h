#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "config.h"

class SessionManager {
    pgw::types::Blacklist m_BlackList;
    std::list<Session> m_sessions;
    pgw::types::Seconds m_sessionTimeoutSec;

public:
    SessionManager(Config config);
    void createSession(std::string_view imsi);
    void removeSession(std::string_view imsi);
    void gracefulShutdown(pgw::types::Rate rate);
};

#endif // SESSION_MANAGER_H