#ifndef PGW_SESSION_MANAGER_H
#define PGW_SESSION_MANAGER_H

#include "ISessionManager.h"
#include "ICdrWriter.h"
#include "types.h"

#include <memory> // Для unique_ptr
#include <unordered_map>

// Менеджер сессий абонентов
// Отвечает за создание, хранение и удаление сессий, взаимодействует
// с системой CDR (Call Detail Records) и поддерживает graceful shutdown

namespace pgw {

class SessionManager : public ISessionManager {
private:
    // Ссылка на систему записи CDR (записывает детали сессий)
    pgw::ICdrWriter& m_cdrWriter;

    // Контейнер активных сессий абонентов
    sessions m_sessions;

    // Ссылка на черный список заблокированных абонентов
    pgw::types::Blacklist& m_blacklist;

    // Время жизни сессий
    const pgw::types::seconds_t m_sessionTimeoutSec;

    // Скорость сессий/секунду для контролируемого удаления
    const pgw::types::rate_t m_shutdownRate;

    // Статистика по обработанным сессиям
    Statistics m_stats;

    // Начало работы менеджера сессий
    std::chrono::steady_clock::time_point m_startTime;

public:
    explicit SessionManager(
        pgw::ICdrWriter& cdrWriter,
        pgw::types::Blacklist& blacklist,
        const pgw::types::seconds_t timeout,
        const pgw::types::rate_t rate
    );
    ~SessionManager();

    bool hasSession(const pgw::types::imsi_t& imsi) const override;

    bool addToBlacklist(const pgw::types::imsi_t& imsi) override;

    size_t countActiveSession() const override;

    const sessions getActiveSessions() const override {return m_sessions;}

    CreateResult createSession(const pgw::types::imsi_t& imsi) override;

    Statistics getStatistics() const override;

    bool terminateSession(const pgw::types::imsi_t& imsi) override;

    // Удаляет все сессии, привысившие таймаут
    void cleanTimeoutSessions();

    // Завершение всех сессий с контролируемой скоростью
    void gracefulShutdown();
};

} // namespace pgw

#endif // PGW_SESSION_MANAGER_H