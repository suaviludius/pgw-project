#include "SessionManager.h"
#include "logger.h"

#include <thread> // Для задержки в graceful shutdown
#include <algorithm>

SessionManager::SessionManager(
    const pgw::types::Blacklist& blacklist,
    const pgw::types::Seconds timeout,
    const pgw::types::Rate rate
) : m_blacklist{blacklist},
    m_sessionTimeoutSec{timeout},
    m_shutdownRate{rate} {
    Logger::info("Session Manager created");
}

SessionManager::~SessionManager(){
    Logger::info("Session Manager deleted");
}

SessionManager::sessions::iterator SessionManager::findSession(pgw::types::ConstImsi imsi) {
    return std::find_if(m_sessions.begin(),m_sessions.end(), // Ищем среди активных сессий imsi пришедшей
    [&imsi](const auto& session){
            return session->getImsi() == imsi;
    });
}

SessionManager::sessions::const_iterator SessionManager::findSession(pgw::types::ConstImsi imsi) const{
    return std::find_if(m_sessions.begin(),m_sessions.end(), // Ищем среди активных сессий imsi пришедшей
    [&imsi](const auto& session){
            return session->getImsi() == imsi;
    });
}

SessionManager::CreateResult SessionManager::createSession(pgw::types::ConstImsi imsi){
    if(m_blacklist.contains(imsi)){
        Logger::warn("Session rejected (blacklisted) for IMSI: " + std::string(imsi));
        return CreateResult::REJECTED_BLACKLIST;
    }

    // Проверяем существует ли сессия
    if (hasSession(imsi)){
        Logger::debug("Session already exists for IMSI: " + std::string(imsi));
        return CreateResult::ALREADY_EXISTS;
    }

    try {
        auto session {std::make_unique<Session>(imsi)};     // Создаём новую сессию через unique_ptr
        bool inserted {m_sessions.add(std::move(session))}; // Перемещаем сессию в контейнер
        if(inserted){
            Logger::info("Session created for IMSI: " + std::string(imsi));
            return CreateResult::CREATED;
        }
        else{
            Logger::error("Session creation failed for IMSI: " + std::string(imsi));
            return CreateResult::ERROR;
        }
    }
    catch (const std::exception& e){
        Logger::error("Session creation failed (exception) for IMSI: " + std::string(imsi) + static_cast<std::string>(e.what()));
        return CreateResult::ERROR;
    }
}

void SessionManager::removeSession(pgw::types::ConstImsi imsi){
    auto it {findSession(imsi)};
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

void SessionManager::gracefulShutdown(){
    Logger::info("Sessions graceful shutdown start");
    // Рассчитываем интервал между удалениями сессий
    // Например, rate = 10 сессий/сек -> интервал = 1000ms / 10 = 100ms на сессию
    const auto delayBetweenSessions = std::chrono::milliseconds(1000 / m_shutdownRate);

    auto it {m_sessions.begin()};
    auto lastDeletionTime = std::chrono::steady_clock::now();
    while (it != m_sessions.end()) {
        auto imsi {(*it)->getImsi()};  // erase делает текущий it невалидным, поэтому сохраняем
        // Задержка для контроля скорости
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastDeletion = now - lastDeletionTime;
        if (timeSinceLastDeletion < delayBetweenSessions) {
            auto calculatedDelay = delayBetweenSessions - timeSinceLastDeletion;
            std::this_thread::sleep_for(calculatedDelay);
        }
        // Удаляем сессию
        it = m_sessions.erase(it);  // erase возвращает следующий валидный итератор
        Logger::session_deleted(imsi);
    }
    Logger::info("Sessions graceful shutdown completed");
}