#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "ISessionManager.h"
#include "Session.h"

#include <memory> // Для unique_ptr

class SessionManager : public ISessionManager {
public:
    using sessions = pgw::types::Container<std::unique_ptr<Session>>;
private:
    pgw::types::Blacklist m_blacklist;
    pgw::types::Seconds m_sessionTimeoutSec;
    pgw::types::Rate m_shutdownRate; // Сессий/секунду для удаления
    sessions m_sessions; // RAII, поэтому используем умные указатели

    std::atomic<bool> m_shutdownRequest{false}; // Запрос на очищение менеджера сессий (принимается из любых потоков)

    sessions::iterator findSession(pgw::types::ConstImsi imsi); // Для внутренних методов auto можно использовать
    sessions::const_iterator findSession(pgw::types::ConstImsi imsi) const; // версия для константных методов
public:
    explicit SessionManager(
        const pgw::types::Blacklist& blacklist,
        const pgw::types::Seconds timeout,
        const pgw::types::Rate rate
    );
    ~SessionManager();

    CreateResult createSession(pgw::types::ConstImsi imsi) override;
    void removeSession(pgw::types::ConstImsi imsi);
    void gracefulShutdown();
    void cleanTimeoutSessions();

    bool hasSession(pgw::types::ConstImsi imsi) const override {return (findSession(imsi) != m_sessions.end());}
    size_t getSessionCount() const override {return m_sessions.size();}
    void requestGracefulShutdown() override {m_shutdownRequest = true; }
};

#endif // SESSION_MANAGER_H