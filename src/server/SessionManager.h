#ifndef PGW_SESSION_MANAGER_H
#define PGW_SESSION_MANAGER_H

#include "ISessionManager.h"
#include "ICdrWriter.h"
#include "Session.h"
#include "types.h"

#include <memory> // Для unique_ptr

// Менеджер сессий абонентов
// Отвечает за создание, хранение и удаление сессий, взаимодействует
// с системой CDR (Call Detail Records) и поддерживает graceful shutdown

class SessionManager : public ISessionManager {
public:
    // Тип контейнера для хранения сессий
    using sessions = pgw::types::Container<std::unique_ptr<Session>>;

private:
    // Ссылка на систему записи CDR (записывает детали сессий)
    ICdrWriter& m_cdrWriter;

    // Контейнер активных сессий абонентов
    sessions m_sessions;

    // Ссылка на черный список заблокированных абонентов
    const pgw::types::Blacklist& m_blacklist;

    // Время жизни сессий
    const pgw::types::Seconds m_sessionTimeoutSec;

    // Скорость сессий/секунду для контролируемого удаления
    const pgw::types::Rate m_shutdownRate;

    // Внутренний метод поиска сессии по IMSI
    sessions::iterator findSession(pgw::types::ConstImsi imsi);
public:
    explicit SessionManager(
        ICdrWriter& cdrWriter,
        const pgw::types::Blacklist& blacklist,
        const pgw::types::Seconds timeout,
        const pgw::types::Rate rate
    );
    ~SessionManager();

    // Проверяет наличие активной сессии для указанного IMSI
    bool hasSession(pgw::types::ConstImsi imsi) const override;

    // Возвращает текущее количество активных сессий
    size_t countActiveSession() const override;

    // Создает новую сессию для абонента
    CreateResult createSession(pgw::types::ConstImsi imsi) override;

    // Удаляет сессию абонента (принудительно)
    void terminateSession(pgw::types::ConstImsi imsi);

    // Удаляет все сессии, привысившие таймаут
    void cleanTimeoutSessions();

    // Завершение всех сессий с контролируемой скоростью
    void gracefulShutdown();
};

#endif // PGW_SESSION_MANAGER_H