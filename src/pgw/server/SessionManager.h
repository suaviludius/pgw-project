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
public:
    // Структура для хранения данных сессии абонента
    struct SessionData {
        // Временная метка создания сессии
        std::chrono::steady_clock::time_point createdTime;
    };

    // Тип контейнера для хранения сессий
    using sessions = std::unordered_map<pgw::types::imsi_t, SessionData>;

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

public:
    explicit SessionManager(
        pgw::ICdrWriter& cdrWriter,
        pgw::types::Blacklist& blacklist,
        const pgw::types::seconds_t timeout,
        const pgw::types::rate_t rate
    );
    ~SessionManager();

    // Проверяет наличие активной сессии для указанного IMSI
    bool hasSession(const pgw::types::imsi_t& imsi) const override;

    // Добавляет IMSI в черный список
    bool addToBlacklist(const pgw::types::imsi_t& imsi) override;

    // Возвращает текущее количество активных сессий
    size_t countActiveSession() const override;

    // Создает новую сессию для абонента
    CreateResult createSession(const pgw::types::imsi_t& imsi) override;

    // Удаляет сессию абонента (принудительно)
    void terminateSession(const pgw::types::imsi_t& imsi);

    // Удаляет все сессии, привысившие таймаут
    void cleanTimeoutSessions();

    // Завершение всех сессий с контролируемой скоростью
    void gracefulShutdown();
};

} // namespace pgw

#endif // PGW_SESSION_MANAGER_H