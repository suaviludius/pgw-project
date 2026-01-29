#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "types.h"
#include "Session.h"

#include <memory> // Для unique_ptr

class SessionManager {
public:
    enum class CreateResult {
        CREATED,
        REJECTED_BLACKLIST,
        ALREADY_EXISTS,
        ERROR
    };
    // Вот тут стоило бы использовать такое:
    // using SessionId = IMSI?. Это будет ключ.
    // using SessionPtr = std::unique_ptr<Session>;
    // using Sessions = std::unordered_map<SessionId, SessionPtr>.
    // Теперь у тебя нет лишнего самописного контейнера, поиск сессии по контейнеру O(1) апроксимировано
    using sessions = pgw::types::Container<std::unique_ptr<Session>>;

private:
    pgw::types::Blacklist m_blacklist;
    pgw::types::Seconds m_sessionTimeoutSec;
    pgw::types::Rate m_shutdownRate;
    sessions m_sessions; // RAII, поэтому используем умные указатели

    // Затем всё это заменилось бы на ф-цию
    // SessionPtr findSession(pgw::types::ConstImsi imsi);
    sessions::iterator findSession(pgw::types::ConstImsi imsi); // Для внутренних методов auto можно использовать
    sessions::const_iterator findSession(pgw::types::ConstImsi imsi) const; // версия для константных методов
public:
    explicit SessionManager(
        const pgw::types::Blacklist& blacklist,
        const pgw::types::Seconds timeout,
        const pgw::types::Rate rate
    );
    ~SessionManager();

    CreateResult createSession(pgw::types::ConstImsi imsi);
    void removeSession(pgw::types::ConstImsi imsi);
    void gracefulShutdown(pgw::types::Rate rate);
    void cleanTimeoutSessions();

    bool hasSession(pgw::types::ConstImsi imsi) const {return (findSession(imsi) != m_sessions.end());}
    size_t getSessionCount() const {return m_sessions.size();}
};

#endif // SESSION_MANAGER_H
