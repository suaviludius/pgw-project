#include "SessionManager.h"
#include "logger.h"

#include <thread>

SessionManager::SessionManager(const Config& config) :
    m_sessionTimeoutSec{config.getSessionTimeoutSec()},
    m_blacklist{std::move(config.getBlacklist())},
    m_shutdownRate{config.getGracefulShutdownRate()} { // Блэк лист в конфиге я больше использовать не собираюсь
    Logger::info("Session Manager created");
}

auto SessionManager::findInSessions(pgw::types::ConstImsi imsi) {
    return std::find_if(m_sessions.begin(),m_sessions.end(), // Ищем среди активных сессий imsi пришедшей
    [&imsi](const auto& session){
            return (session->getImsi() == imsi);
    });
}

void SessionManager::createSession(pgw::types::ConstImsi imsi){
    if(m_blacklist.contains(imsi)){
        Logger::session_rejected(imsi, "blacklist contain imsi");
        return;
    }

    // Проверяем существует ли сессия
    auto it {findInSessions(imsi)};
    if (it != m_sessions.end()){
        Logger::debug("Session already exists for IMSI:" + static_cast<std::string>(imsi));
        return;
    }

    try {
        auto session {std::make_unique<Session>(imsi)}; // Создаём новую сессию через unique_ptr
        bool inserted {m_sessions.add(std::move(session))}; // Перемещаем сессию в контейнер
        if(inserted){
            Logger::session_created(imsi);
        }
        else{
            Logger::error("Filed to add session for IMSI:" + static_cast<std::string>(imsi));
        }
    }
    catch (const std::exception& e){
        Logger::error("Filed to create session for IMSI: {}. {}", static_cast<std::string>(imsi), e.what());
    }
    return;
}

void SessionManager::removeSession(pgw::types::ConstImsi imsi){
    auto it {findInSessions(imsi)};
    if (it != m_sessions.end()){
        m_sessions.erase(it);
        Logger::session_deleted(imsi);
    } else {
        Logger::debug("Session not found for removal IMSI: " + std::string(imsi));
    }
}

void SessionManager::cleanTimeoutSessions(){
    auto it {m_sessions.begin()};
    while (it != m_sessions.end()) {
        if ((*it)->getAge() >= m_sessionTimeoutSec) {
            auto imsi {(*it)->getImsi()};  // erase делает текущий it невалидным, поэтому сохраняем
            it = m_sessions.erase(it);  // erase возвращает следующий валидный итератор
            Logger::session_deleted(imsi);
        } else {
            ++it;
        }
    }
}

void SessionManager::gracefulShutdown(pgw::types::Rate rate){
    Logger::info("Graceful shutdown start");
    auto it {m_sessions.begin()};
    while (it != m_sessions.end()) {
        auto imsi {(*it)->getImsi()};  // erase делает текущий it невалидным, поэтому сохраняем
        it = m_sessions.erase(it);  // erase возвращает следующий валидный итератор
        Logger::session_deleted(imsi);
        // Задержка для контроля скорости
        if (m_shutdownRate > 0 && it != m_sessions.end()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_shutdownRate));
        }
    }
    Logger::info("Graceful shutdown completed");
}