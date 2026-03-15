#include "UdpServer.h"
#include "UdpSocket.h"
#include "SocketFactory.h"
#include "logger.h"
#include "validation.h"

#include <cstring>

namespace pgw {

UdpServer::UdpServer(ISessionManager& sessionManager,
                     pgw::types::constIp_t ip,
                     pgw::types::port_t port,
                     std::unique_ptr<IUdpSocket> socket)
try : m_sessionManager{sessionManager},
      m_socket{socket ? std::move(socket) : SocketFactory::createUdp()},
      m_ip{ip},
      m_port{port},
      m_running{false}{
    LOG_INFO("UDP server initialized");
}
catch(const std::exception& e){
    LOG_ERROR("UDP server initialization failed: {}", e.what());
    throw std::runtime_error("UDP server initialization failed");
}

UdpServer::~UdpServer(){
    if(m_running){
        stop();
    }
    LOG_INFO("UDP server deleted");
}

void UdpServer::start(){
    if(m_running) {
        LOG_WARN("UDP server already running");
        return;
    }

    try{
        LOG_INFO("Starting UDP server...");
        m_socket->bind(m_ip, m_port);
        m_running = true;
    }
    catch (const std::exception& e){
        LOG_ERROR("UDP server start failed {} : {} - {}", m_ip, m_port, e.what());
        throw std::runtime_error("UDP server start failed");
    }

    LOG_INFO("UDP server started");
}

void UdpServer::stop(){
    if(m_running) {
        m_running = false;
        LOG_INFO("UDP server stopped");
    }
}

// Метод рассчитан на использование с менеджером poll, epoll, select
void UdpServer::handler(){
    try{
        // Читаем все доступные пакеты
        while(true){
            auto packet = m_socket->receive();

            if(packet.data.empty()) break;
            
            std::string answer {"rejected"};
            std::string imsi = packet.data;

            // Создаем новую сессию только для валидных imsi
            if (validateImsi(imsi)) {
                switch (m_sessionManager.createSession(imsi)) {
                    case ISessionManager::CreateResult::CREATED:
                        answer = "created";
                        break;
                    case ISessionManager::CreateResult::REJECTED_BLACKLIST:
                    case ISessionManager::CreateResult::ALREADY_EXISTS:
                    case ISessionManager::CreateResult::ERROR:
                    default: break;
                }
            }

            m_socket->send(answer, packet.senderAddr);
        }
    }
    catch(const std::exception& e){
        LOG_ERROR("UDP server running error: {}", e.what());
    }
}

bool UdpServer::validateImsi(const std::string& imsi){
    if (!pgw::validation::isValidImsi(imsi)) {
        LOG_WARN("UDP server received invalid imsi: {}", imsi);
        return false;
    }
    return true;
}

} // namespace pgw
