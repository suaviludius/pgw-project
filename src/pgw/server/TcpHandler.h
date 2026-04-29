#ifndef PGW_TCP_HANDLER_H
#define PGW_TCP_HANDLER_H

#include "TcpSerializer.h"
#include "ISessionManager.h"
#include "DatabaseManager.h"

#include <atomic>

// Обработчик команд от TCP клиентов
// Отвечает за разбор протокола, выполнение команд и формирование ответов

namespace pgw {


class TcpHandler {
private:
    // Ссылка на менеджер сессий (ассоциация)
    ISessionManager& m_sessionManager;

    // Указатель на раздельное владение базой данный (создается извне)
    std::shared_ptr<DatabaseManager> m_dbManager;

    // Ссылка на атомик завершения сессии
    std::atomic<bool>& m_shutdownRequest;

public:
    TcpHandler(ISessionManager& sessionManager,
               std::shared_ptr<DatabaseManager> dbManager,
               std::atomic<bool>& shutdownRequest);
    ~TcpHandler() = default;

    protocol::Message handle(const protocol::Message& request);

private:
    protocol::Message handleGetStats();
    protocol::Message handleGetSessions();
    protocol::Message handleGetCdr(const nlohmann::json& params);
    protocol::Message handleStartSession(const nlohmann::json& params);
    protocol::Message handleStopSession(const nlohmann::json& params);
    protocol::Message handleShutdown();
};


} // namespace pgw

#endif // PGW_TCP_HANDLER_H