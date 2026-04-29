#ifndef I_SESSION_MANAGER_H
#define I_SESSION_MANAGER_H

#include "types.h"

#include <unordered_map>

struct ISessionManager {
    // Результаты выполнения запроса на создание сессии
    enum class CreateResult {
        CREATED,
        REJECTED_BLACKLIST,
        ALREADY_EXISTS,
        ERROR
    };

    // Структура хранения статистических данных работы менеджера сессий
    struct Statistics {
        size_t activeSessions = 0;
        size_t createdSessions = 0;
        size_t rejectedSessions = 0;
        size_t expiredSessions = 0;
        size_t terminateSessions = 0;
        size_t totalSessions = 0;
        size_t uptimeSeconds = 0;
    };

    // Структура для хранения данных сессии абонента
    struct SessionData {
        // Временная метка создания сессии
        std::chrono::steady_clock::time_point createdTime;
    };

    // Тип контейнера для хранения сессий
    using sessions = std::unordered_map<pgw::types::imsi_t, SessionData>;

    virtual ~ISessionManager() = default;

    // Создает новую сессию для абонента
    virtual CreateResult createSession(const pgw::types::imsi_t& imsi) = 0;

    // Проверяет наличие активной сессии для указанного IMSI
    virtual bool hasSession(const pgw::types::imsi_t& imsi) const = 0;

    // Добавляет IMSI в черный список
    virtual bool addToBlacklist(const pgw::types::imsi_t& imsi) = 0;

    // Возвращает текущее количество активных сессий
    virtual size_t countActiveSession() const = 0;

    // Возвращает активные сессии
    virtual const sessions getActiveSessions() const = 0;

    // Возвращает cтатистику работы менеджера сессий
    virtual Statistics getStatistics() const = 0;

    // Удаляет сессию абонента (принудительно)
    virtual bool terminateSession(const pgw::types::imsi_t& imsi) = 0;
};

#endif // I_SESSION_MANAGER_H