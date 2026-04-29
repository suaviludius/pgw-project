#include "TcpHandler.h"
#include "logger.h"
#include "validation.h"

namespace pgw {

TcpHandler::TcpHandler(ISessionManager& sessionManager,
               std::shared_ptr<DatabaseManager> dbManager,
               std::atomic<bool>& shutdownRequest)
            :  m_sessionManager{sessionManager},
               m_dbManager{std::move(dbManager)},
               m_shutdownRequest{shutdownRequest}{
    LOG_INFO("TCP command handler initit");
}

protocol::Message TcpHandler::handle(const protocol::Message& request){
    protocol::Command command = static_cast<protocol::Command>(request.header.command);

    LOG_DEBUG("Handling command: {}", static_cast<int>(command));

    switch (command) {
        case protocol::Command::GET_STATS:
            return handleGetStats();

        case protocol::Command::GET_SESSIONS:
            return handleGetSessions();

        case protocol::Command::GET_CDR: {
            auto params = TcpSerializer::getJsonData(request);
            return handleGetCdr(params);
        }

        case protocol::Command::START_SESSION: {
            auto params = TcpSerializer::getJsonData(request);
            return handleStartSession(params);
        }

        case protocol::Command::STOP_SESSION: {
            auto params = TcpSerializer::getJsonData(request);
            return handleStopSession(params);
        }

        case protocol::Command::SHUTDOWN:
            return handleShutdown();

        default:
            LOG_WARN("Unknown command: {}", static_cast<int>(command));
            protocol::Message response;
            response.header.command = request.header.command;
            response.header.status = protocol::Status::INVALID_COMMAND;
            response.header.length = 0;
            return response;
    }
}

protocol::Message TcpHandler::handleGetStats(){
    nlohmann::json response;

    const auto stats = m_sessionManager.getStatistics();

    response["active_sessions"] = stats.activeSessions;
    response["created_sessions"] = stats.createdSessions;
    response["expired_sessions"] = stats.expiredSessions;
    response["rejected_sessions"] = stats.rejectedSessions;
    response["terminate_sessions"] = stats.terminateSessions;
    response["total_sessions"] = stats.totalSessions;
    response["uptime_seconds"] = stats.uptimeSeconds;

    return TcpSerializer::createJsonMsg(protocol::Command::GET_STATS, protocol::Status::OK,response);
}

protocol::Message TcpHandler::handleGetSessions() {

    nlohmann::json response;
    nlohmann::json sessionsArray = nlohmann::json::array();

    // Получаем список активных сессий
    const auto activeSessions = m_sessionManager.getActiveSessions();  // Нужно добавить этот метод
    auto now = std::chrono::steady_clock::now();

    // TODO: сделать вывод не возраста сессии, а начала и планируемого времени завершения
    for (const auto& session : activeSessions) {
        nlohmann::json sessionJson;
        sessionJson["imsi"] = session.first;
        sessionJson["age"] = std::chrono::duration_cast<std::chrono::seconds>(now - session.second.createdTime).count();
        sessionsArray.push_back(sessionJson);
    }

    response["sessions"] = sessionsArray;
    response["count"] = sessionsArray.size();

    return TcpSerializer::createJsonMsg(protocol::Command::GET_SESSIONS, protocol::Status::OK,response);
}


protocol::Message TcpHandler::handleGetCdr(const nlohmann::json& params) {
    if (!m_dbManager || !m_dbManager->isConnected()) {
        return TcpSerializer::createJsonMsg(protocol::Command::GET_CDR,protocol::Status::ERROR);
    }

    size_t limit;
    if (params.contains("limit") && params["limit"].is_number()) {
        limit = params["limit"].get<size_t>();
    }

    // Получаем CDR записи из БД
    auto records = m_dbManager->getRecentCdr(limit);

    nlohmann::json response;
    nlohmann::json recordsArray = nlohmann::json::array();

    for (const auto& record : records) {
        nlohmann::json recordJson;
        recordJson["timestamp"] = record.timestamp;
        recordJson["imsi"] = record.imsi;
        recordJson["action"] = record.action;
        recordsArray.push_back(recordJson);
    }

    response["records"] = recordsArray;
    response["count"] = recordsArray.size();
    // Если лимит превышает допустимый, определенный в менеджере БД, то запрашиваемый лимит будет усекаться
    response["limit"] = limit;

    return TcpSerializer::createJsonMsg(protocol::Command::GET_CDR, protocol::Status::OK, response);
}


protocol::Message TcpHandler::handleStartSession(const nlohmann::json& params) {
    // Проверяем наличие правильного IMSI
    std::string imsi;

    if (params.contains("imsi") || params["imsi"].is_string()) {
        imsi = params["imsi"].get<std::string>();
    }

    if (imsi.empty() || !validation::isValidImsi(imsi)){
        return TcpSerializer::createJsonMsg(protocol::Command::START_SESSION,protocol::Status::INVALID_PARAMS); 
    }

    // Создаём сессию
    auto result = m_sessionManager.createSession(imsi);

    nlohmann::json response;

    switch (result) {
        case ISessionManager::CreateResult::CREATED:
            response["result"] = "created";
            response["status"] = "success";
            break;

        case ISessionManager::CreateResult::REJECTED_BLACKLIST:
            response["result"] = "rejected";
            response["status"] = "failed";
            break;

        case ISessionManager::CreateResult::ALREADY_EXISTS:
            response["result"] = "exists";
            response["status"] = "failed";
            break;

        default:
            response["result"] = "error";
            response["status"] = "failed";
            break;
    }

    return TcpSerializer::createJsonMsg(protocol::Command::START_SESSION,protocol::Status::OK,response);
}


protocol::Message TcpHandler::handleStopSession(const nlohmann::json& params) {
        // Проверяем наличие правильного IMSI
    std::string imsi;

    if (params.contains("imsi") || params["imsi"].is_string()) {
        imsi = params["imsi"].get<std::string>();
    }

    if (imsi.empty() || !validation::isValidImsi(imsi)){
        return TcpSerializer::createJsonMsg(protocol::Command::STOP_SESSION,protocol::Status::INVALID_PARAMS);
    }

    // Удаляем сессию
    bool removed = m_sessionManager.terminateSession(imsi);

    nlohmann::json response;

    if (removed) {
        response["result"] = "deleted";
        response["status"] = "success";
    } else {
        response["result"] = "not_found";
        response["status"] = "failed";
    }

    return TcpSerializer::createJsonMsg(protocol::Command::STOP_SESSION,protocol::Status::OK,response);
}

protocol::Message TcpHandler::handleShutdown() {
    LOG_INFO("Shutdown command received by TCP");

    // Устанавливаем флаг для главного цикла
    m_shutdownRequest.store(true);

    nlohmann::json response;
    response["message"] = "Shutdown request was install";

    return TcpSerializer::createJsonMsg(protocol::Command::SHUTDOWN,protocol::Status::OK,response);
}

} // namespace pgw