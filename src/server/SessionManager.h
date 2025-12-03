#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "config.h"
#include "Session.h"

#include <unordered_set>

class SessionManager {
    pgw::types::Blacklist m_blacklist;
    std::unordered_set<Session> m_sessions; // Используем неупорядоченное(упорядоченность не нужна) множество(без повторений)
    pgw::types::Seconds m_sessionTimeoutSec;
    pgw::types::Rate m_shutdownRate;

    auto findInSessions(pgw::types::ConstImsi imsi);
public:
    explicit SessionManager(const Config& config);
    void createSession(pgw::types::ConstImsi imsi);
    void removeSession(pgw::types::ConstImsi imsi);
    void gracefulShutdown(pgw::types::Rate rate);
    void cleanTimeoutSessions();
};

#endif // SESSION_MANAGER_H