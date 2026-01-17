#include "SessionManager.h"
#include "logger.h"

#include <algorithm>
#include <thread>   // std::thread

SessionManager::SessionManager(
    const pgw::types::Blacklist& blacklist,
    const pgw::types::Seconds timeout,
    const pgw::types::Rate rate
) : m_blacklist{blacklist},
    m_sessionTimeoutSec{timeout},
    m_shutdownRate{rate} {
    LOG_INFO("Session Manager initialized");
}

SessionManager::~SessionManager(){
    LOG_INFO("Session Manager deleted");
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
        LOG_WARN("Session rejected (blacklisted) for IMSI: {}" , imsi);
        return CreateResult::REJECTED_BLACKLIST;
    }

    // Проверяем существует ли сессия
    if (hasSession(imsi)){
        LOG_DEBUG("Session already exists for IMSI: {} ", imsi);
        return CreateResult::ALREADY_EXISTS;
    }

    try {
        auto session {std::make_unique<Session>(imsi)};     // Создаём новую сессию через unique_ptr
        bool inserted {m_sessions.add(std::move(session))}; // Перемещаем сессию в контейнер
        if(inserted){
            LOG_INFO("Session created for IMSI: {} ", imsi);
            return CreateResult::CREATED;
        }
        else{
            LOG_ERROR("Session creation failed for IMSI: {} ", imsi);
            return CreateResult::ERROR;
        }
    }
    catch (const std::exception& e){
        LOG_ERROR("Session creation failed (exception) for IMSI: {}. {}", imsi, e.what());
        return CreateResult::ERROR;
    }
}

void SessionManager::removeSession(pgw::types::ConstImsi imsi){
    auto it {findSession(imsi)};
    if (it != m_sessions.end()){
        m_sessions.erase(it);
        LOG_INFO("SESSION_DELETED imsi: {}",imsi);
    } else {
        LOG_DEBUG("Session not found for removal IMSI: {}" , imsi);
    }
}

void SessionManager::cleanTimeoutSessions(){
    auto it {m_sessions.begin()};
    while (it != m_sessions.end()) {
        if ((*it)->getAge() >= m_sessionTimeoutSec) {
            auto imsi {(*it)->getImsi()};  // erase делает текущий it невалидным, поэтому сохраняем
            it = m_sessions.erase(it);  // erase возвращает следующий валидный итератор
            LOG_INFO("SESSION_DELETED imsi: {}",imsi);
        } else {
            ++it;
        }
    }
}

void SessionManager::gracefulShutdown(){
    LOG_INFO("Sessions graceful shutdown start");
    // Рассчитываем интервал между удалениями сессий
    // Например, rate = 10 сессий/сек -> интервал = 1000ms / 10 = 100ms на сессию
    const auto delayBetweenSessions = std::chrono::milliseconds(1000 / m_shutdownRate);

    auto it {m_sessions.begin()};
    auto lastDeletionTime = std::chrono::steady_clock::now();
    while (it != m_sessions.end()) {
        auto imsi {(*it)->getImsi()};
        // Задержка для контроля скорости
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastDeletion = now - lastDeletionTime;
        if (timeSinceLastDeletion < delayBetweenSessions) {
            auto calculatedDelay = delayBetweenSessions - timeSinceLastDeletion;
            std::this_thread::sleep_for(calculatedDelay);
        }
        // Удаляем сессию. erase возвращает следующий валидный итератор
        it = m_sessions.erase(it);
        LOG_INFO("SESSION_DELETED imsi: {}", imsi);
    }
    if (m_sessions.empty()) {
        LOG_INFO("Sessions graceful shutdown completed");
    }
    else {
        LOG_INFO("Sessions graceful shutdown error");
    }
}