#include "SessionManager.h"

#include "logger.h"
#include "validation.h"

#include <algorithm>
#include <thread>

namespace pgw {

SessionManager::SessionManager(
    pgw::ICdrWriter& cdrWriter,
    pgw::types::Blacklist& blacklist,
    const pgw::types::seconds_t timeout,
    const pgw::types::rate_t rate)
    : m_cdrWriter{cdrWriter},
      m_blacklist{blacklist},
      m_sessionTimeoutSec{timeout},
      m_shutdownRate{rate},
      m_startTime{std::chrono::steady_clock::now()}{
    LOG_INFO("Session Manager initialized");
}

SessionManager::~SessionManager(){
    LOG_INFO("Session Manager deleted");
}

size_t SessionManager::countActiveSession() const {
    return m_sessions.size();
}

bool SessionManager::addToBlacklist(const pgw::types::imsi_t& imsi) {
    if (pgw::validation::isValidImsi(imsi)){
        return m_blacklist.insert(imsi).second;
    }
    return false;
}

bool SessionManager::hasSession(const pgw::types::imsi_t& imsi) const {
    return m_sessions.find(imsi) != m_sessions.end();
}

SessionManager::CreateResult SessionManager::createSession(const pgw::types::imsi_t& imsi){
    if(m_blacklist.find(imsi) != m_blacklist.end()){
        m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_REJECTED);
        m_stats.rejectedSessions++;
        m_stats.totalSessions++;
        LOG_WARN("Session rejected (blacklisted) for IMSI: {}" , imsi);
        return CreateResult::REJECTED_BLACKLIST;
    }

    if (hasSession(imsi)){
        LOG_DEBUG("Session already exists for IMSI: {} ", imsi);
        return CreateResult::ALREADY_EXISTS;
    }

    try {
        // Создаём новую сессию и пытаемся сделать ход имплейсом
        auto [it, inserted] = m_sessions.try_emplace(
            imsi,                                   // ключ
            SessionData{                            // значение
                std::chrono::steady_clock::now()    // createdTime
            }
        );

        if(inserted){
            m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_CREATED);
            m_stats.createdSessions++;
            m_stats.totalSessions++;
            m_stats.activeSessions = m_sessions.size();
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

bool SessionManager::terminateSession(const pgw::types::imsi_t& imsi){
    // Ищем сессию в мапе
    auto it = m_sessions.find(imsi);

    // Удаляем сессию, если итератор на сессию находится в пределах контейнера
    if (it != m_sessions.end()){
        m_sessions.erase(it);
        m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_DELETED);
        m_stats.terminateSessions++;
        m_stats.activeSessions = m_sessions.size();
        LOG_INFO("Session deleted for imsi: {}",imsi);
        return true;
    } else {
        LOG_DEBUG("Session not found for removal IMSI: {}" , imsi);
        return false;
    }
}

void SessionManager::cleanTimeoutSessions(){
    // Тип часов для измерения интервалов времени
    auto now = std::chrono::steady_clock::now();

    auto it = m_sessions.begin();

    while (it != m_sessions.end()) {
        auto age = now - it->second.createdTime;

        if (age >= m_sessionTimeoutSec) {
            auto imsi = it->first;      // Сохраняем IMSI перед удалением;   // erase делает текущий it невалидным, поэтому сохраняем
            it = m_sessions.erase(it);  // erase возвращает следующий валидный итератор
            m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_DELETED);
            m_stats.expiredSessions++;
            m_stats.activeSessions = m_sessions.size(); // Так немного дольше, зато надежнее
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
        auto imsi = it->first;  // Сохраняем IMSI перед удалением

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

        it = std::next(it);
        // Удаляем сессию
        terminateSession(imsi);

        // TODO: До сих пор не знаю как поступить:
        // 1) сделать перегруженную фукнцию для терминейта
        // 2) оставить здесь удаление сессии
        // 3) передавать в терминейт итератор, а не imsi
        // С дргуой стороны, сложность поиска в unrderd_map O(1),
        // а вызов этого шатдауна явление редкое, так что может и так пойдет

        //it = m_sessions.erase(it);
        //m_cdrWriter.writeAction(imsi, ICdrWriter::SESSION_DELETED);
        //LOG_INFO("Session deleted for imsi: {}", imsi);
    }

    if (m_sessions.empty()) {
        m_stats.activeSessions = 0;
        LOG_INFO("Sessions graceful shutdown completed");
    } else {
        LOG_INFO("Sessions graceful shutdown error");
    }
}


// TODO: стоит сделать мьютекс для метода, в случае вынесения
// TCP и UDP серверов в разные потоки
SessionManager::Statistics SessionManager::getStatistics() const {
    Statistics stats = m_stats;
    // Актуальное значение на момент вызова метода
    stats.activeSessions = m_sessions.size();
    stats.uptimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::steady_clock::now() - m_startTime).count();
    return stats;
}


} // namespace pgw
