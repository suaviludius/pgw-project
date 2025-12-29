#include "Socket.h" // Чтобы создать объект Socket()
#include "UdpServer.h"
#include "logger.h"

#include <algorithm> // all_of()
#include <cstring>

UdpServer::UdpServer(ISessionManager& sessionManager,
                     ICdrWriter& cdrWriter,
                     std::unique_ptr<ISocket> socket,
                     pgw::types::ConstIp ip,
                     pgw::types::Port port)
    : m_sessionManager{sessionManager},
      m_cdrWriter{cdrWriter},
      m_socket{std::move(socket)},
      m_ip{ip},
      m_port{port},
      m_running{false}{
    Logger::info("UDP server initialized");
}

UdpServer::~UdpServer(){
    stop();
}

void UdpServer::start(){
    if(m_running) {
        Logger::warn("UDP server already running");
        return;
    }
    try{
        m_socket->bind(m_ip,m_port);
        m_running = true;
    }
    catch (const std::exception& e){
        Logger::error("UDP server start failed " + std::string(m_ip) + ":" + std::to_string(m_port) + " - " + std::string(e.what()));
        // Пробрасываем с дополнительным контекстом
        std::throw_with_nested(std::runtime_error("UDP server start failed"));
    }
    Logger::info("UDP server sucseccdully start");
    // run();
}

void UdpServer::stop(){
    if(!m_running) return;
    m_running = false;
    Logger::info("UDP server stopped");
}

// Метод раcсчитан на использование с менеджером poll, epoll, select
void UdpServer::run(){
    if(!m_running) return;
    try{
        // Читаем все пакеты
        while(true){
            auto packet = m_socket->receive();
            if(!packet.data.empty()){
                std::string answer {"rejected"};
                std::string imsi = packet.data;
                // Валидация
                if (!validateImsi(imsi)) {
                    m_socket->send(answer, packet.senderAddr);
                    break;
                }

                switch (m_sessionManager.createSession(imsi)) {
                    case ISessionManager::CreateResult::CREATED:
                        answer = "created";
                        m_cdrWriter.writeAction(imsi, answer);
                        break;
                    case ISessionManager::CreateResult::REJECTED_BLACKLIST:
                        m_cdrWriter.writeAction(imsi, answer);
                        break;
                    case ISessionManager::CreateResult::ALREADY_EXISTS:
                    case ISessionManager::CreateResult::ERROR:
                    default: break;
                }
                m_socket->send(answer, packet.senderAddr);
            }
        }
    }
    catch(const std::exception& e){
        Logger::error("UDP server runninig error: " + std::string(e.what()));
    }
}

bool UdpServer::validateImsi(const std::string& imsi){
    if (imsi.length() != 15) {
        Logger::warn("UDP server receive imsi whith invalid size: " + std::string(imsi));
        return false;
    }
    if (!std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        Logger::warn("UDP server receive imsi whith invalid symbols: " + std::string(imsi));
        return false;
    }
    return true;
}