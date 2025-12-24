#include "UdpServer.h"
#include "logger.h"

#include <algorithm> // all_of()
#include <cstring>


UdpServer::UdpServer(SessionManager& sessionManager,
                     CdrWriter& cdrWriter,
                     pgw::types::ConstIp ip,
                     pgw::types::Port port)
    : m_sessionManager{sessionManager},
      m_cdrWriter{cdrWriter}{
    try{
        m_socket = Socket();
        m_socket.bind(ip,port);
    }
    catch (const std::exception& e){
        Logger::error("UDP server initialised failed " + std::string(ip) + ":" + std::to_string(port) + " - " + std::string(e.what()));
        // Пробрасываем с дополнительным контекстом
        std::throw_with_nested(std::runtime_error("UDP server initialization failed"));
    }
    Logger::info("UDP server initialised");
}

UdpServer::~UdpServer(){
    stop();
}

void UdpServer::start(){
    if(m_running) {
        Logger::warn("UDP server already running");
        return;
    }
    m_running = true;
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
        auto packet = m_socket.recieve();
        if(!packet.data.empty()){
            std::string answer {"rejected"};
            std::string imsi = packet.data;
            // Валидация
            if (validateImsi(imsi)) {
                m_socket.send(answer, packet.senderAddr);
                return;
            }

            switch (m_sessionManager.createSession(imsi)) {
                case SessionManager::CreateResult::CREATED:
                    answer = "created";
                    m_cdrWriter.writeAction(imsi, answer);
                    break;
                case SessionManager::CreateResult::REJECTED_BLACKLIST:
                    m_cdrWriter.writeAction(imsi, answer);
                case SessionManager::CreateResult::ALREADY_EXISTS:
                case SessionManager::CreateResult::ERROR:
                default: break;
            }
            m_socket.send(answer, packet.senderAddr);
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
    return ;
}