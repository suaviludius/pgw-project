#include "SessionManager.h"
#include "logger.h"

SessionManager::SessionManager(const Config& config) :
    m_sessionTimeoutSec{config.getSessionTimeoutSec()},
    m_blacklist{std::move(config.getBlacklist())},
    m_shutdownRate{config.getGracefulShutdownRate()} { // Блэк лист в конфиге я больше использовать не собираюсь
    Logger::info("Session Manager created");
}

void SessionManager::createSession(pgw::types::ConstImsi imsi){
    if(m_blacklist.contains(imsi)){
        Logger::session_rejected(imsi,"blacklist contain imsi");
        return;
    }
    else{
        auto it {findInSessions(imsi)};
        if (it != m_sessions.end()){
            Logger::debug("Session already exists for IMSI:" + static_cast<std::string>(imsi));
            return;
        }
        auto [iterator, inserted] = m_sessions.insert(std::move(Session(imsi))); // Чтобы лишний раз не создавать копии, можно переместить
        if(!inserted){
            Logger::error("Filed to create session for IMSI:" + static_cast<std::string>(imsi));
        }
        Logger::session_created(imsi);
        return;
    }
}

auto SessionManager::findInSessions(pgw::types::ConstImsi imsi) {
    return std::find_if(m_sessions.begin(),m_sessions.end(), // Ищем среди активных сессий imsi пришедшей
    [&imsi](const Session& session){
            return session.getImsi() == imsi;
    });
}

void SessionManager::removeSession(pgw::types::ConstImsi imsi){
    auto it {findInSessions(imsi)};
    if (it != m_sessions.end()){
        m_sessions.erase(it);
        Logger::session_deleted(imsi);
    }
}

void SessionManager::cleanTimeoutSessions(){
    auto it {m_sessions.begin()};
    while (it != m_sessions.end()) {
        if (it->getAge() >= m_sessionTimeoutSec) {
            auto imsi {it->getImsi()};  // erase делает текущий it невалидным, поэтому сохраняем
            it = m_sessions.erase(it);  // erase возвращает следующий валидный итератор
            Logger::session_deleted(imsi);
        } else {
            ++it;
        }
    }
}

void SessionManager::gracefulShutdown(pgw::types::Rate rate){
    auto it {m_sessions.begin()};
    while (it != m_sessions.end()) {
        auto imsi {it->getImsi()};  // erase делает текущий it невалидным, поэтому сохраняем
        it = m_sessions.erase(it);  // erase возвращает следующий валидный итератор
        Logger::session_deleted(imsi);
    }
    Logger::info("Graceful shutdown completed");
}