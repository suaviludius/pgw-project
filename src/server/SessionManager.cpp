#include "server/SessionManager.h"

#include "common/logger.h"

#include <algorithm>
#include <thread>

SessionManager::SessionManager(
    ICdrWriter& cdrWriter,
    const pgw::types::Blacklist& blacklist,
    const pgw::types::Seconds timeout,
    const pgw::types::Rate rate)
    : m_cdrWriter{cdrWriter},
      m_blacklist{blacklist},
      m_sessionTimeoutSec{timeout},
      m_shutdownRate{rate} {
    LOG_INFO("Session Manager initialized");
}

SessionManager::~SessionManager(){
    LOG_INFO("Session Manager deleted");
}

size_t SessionManager::countActiveSession() const {
    return m_sessions.size();
}

SessionManager::sessions::iterator SessionManager::findSession(pgw::types::ConstImsi imsi) {
    return std::find_if(m_sessions.begin(),m_sessions.end(),
         // Ищем совпадения по imsi среди активных сессий
        [&imsi](const auto& session){
            return session->getImsi() == imsi;
        });
}

bool SessionManager::hasSession(pgw::types::ConstImsi imsi) const {
    return std::find_if(m_sessions.begin(),m_sessions.end(),
        // Ищем совпадения по imsi среди активных сессий
        [&imsi](const auto& session){
            return session->getImsi() == imsi;
        }) != m_sessions.end();
}

SessionManager::CreateResult SessionManager::createSession(pgw::types::ConstImsi imsi){
    if(m_blacklist.contains(imsi)){
        m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_REJECTED);
        LOG_WARN("Session rejected (blacklisted) for IMSI: {}" , imsi);
        return CreateResult::REJECTED_BLACKLIST;
    }

    if (hasSession(imsi)){
        LOG_DEBUG("Session already exists for IMSI: {} ", imsi);
        return CreateResult::ALREADY_EXISTS;
    }

    try {
        // Создаём новую сессию через unique_ptr
        auto session {std::make_unique<Session>(imsi)};

        // Перемещаем сессию в контейнер и принимаем успех операции
        bool inserted {m_sessions.add(std::move(session))};

        if(inserted){
            m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_CREATED);
            LOG_INFO("Session created for IMSI: {} ", imsi);
            return CreateResult::CREATED;
        }
        else{
            m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_ERROR);
            LOG_ERROR("Session creation failed for IMSI: {} ", imsi);
            return CreateResult::ERROR;
        }
    }
    catch (const std::exception& e){
        LOG_ERROR("Session creation failed (exception) for IMSI: {}. {}", imsi, e.what());
        return CreateResult::ERROR;
    }
}

void SessionManager::terminateSession(pgw::types::ConstImsi imsi){
    auto it = findSession(imsi);

    // Удаляем сессию, если итератор на сессию находится в пределах контейнера
    if (it != m_sessions.end()){
        m_sessions.erase(it);
        m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_DELETED);
        LOG_INFO("SESSION_DELETED imsi: {}",imsi);
    } else {
        LOG_DEBUG("Session not found for removal IMSI: {}" , imsi);
    }
}

void SessionManager::cleanTimeoutSessions(){
    auto it {m_sessions.begin()};

    while (it != m_sessions.end()) {
        if ((*it)->getAge() >= m_sessionTimeoutSec) {
            auto imsi {(*it)->getImsi()};   // erase делает текущий it невалидным, поэтому сохраняем
            it = m_sessions.erase(it);      // erase возвращает следующий валидный итератор
            m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_DELETED);
            LOG_INFO("SESSION_DELETED imsi: {}",imsi);
        } else {
            ++it;
        }
    }
}

void SessionManager::gracefulShutdown(){
    LOG_INFO("Sessions graceful shutdown start");

    // Рассчитываем интервал между удалениями сессий
    // Например, rate = 10 сессий/сек -> интервал = 1000ms/10 = 100ms на сессию
    const auto delayBetweenSessions = std::chrono::milliseconds(1000 / m_shutdownRate);

    auto lastDeletionTime = std::chrono::steady_clock::now();

    auto it {m_sessions.begin()};

    while (it != m_sessions.end()) {
        auto imsi {(*it)->getImsi()};

        // Начало отсчета для создания задержки
        auto now = std::chrono::steady_clock::now();

        // Создаем задержку между удалениями сессий
        auto timeSinceLastDeletion = now - lastDeletionTime;
        if (timeSinceLastDeletion < delayBetweenSessions) {
            // Приводим временной интервал к нужному размеру
            auto calculatedDelay = delayBetweenSessions - timeSinceLastDeletion;
            std::this_thread::sleep_for(calculatedDelay);
            lastDeletionTime = std::chrono::steady_clock::now();
        }

        // Удаляем сессию
        it = m_sessions.erase(it);
        m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_DELETED);
        LOG_INFO("SESSION_DELETED imsi: {}", imsi);
    }

    if (m_sessions.empty()) {
        LOG_INFO("Sessions graceful shutdown completed");
    } else {
        LOG_INFO("Sessions graceful shutdown error");
    }
}