#include "UdpServer.h"
#include "logger.h"

#include <algorithm> // all_of()
#include <cstring>


UdpServer::UdpServer(SessionManager& sessionManager,
                     CdrWriter& cdrWriter,
                     pgw::types::ConstIp ip,
                     pgw::types::Port port)
    : m_sessionManager{sessionManager},
      m_cdrWriter{cdrWriter},
      m_ip{ip},
      m_port{port}{
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
    try{
        m_socket = Socket();
        m_socket.bind(m_ip,m_port);
        m_running = true;
        Logger::info("UDP server sucseccdully start");
        // run();
    }
    catch(const std::exception& e){
        m_running = false;
        Logger::error("UDP server start failed. " + static_cast<std::string>(e.what()));
    }
}

void UdpServer::stop(){
    if(!m_running) return;
    m_running = false;
    Logger::info("UDP server stopped");
}


void UdpServer::run(){
    while(m_running){
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
            Logger::error("UDP server error: " + std::string(e.what()));
        }
    }
}

bool UdpServer::validateImsi(const std::string& imsi){
    if (imsi.length() != 15) {
        Logger::warn("Invalid UDP imsi size for IMSI:" + );
        return false;
    }
    if (!std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        Logger::warn("Invalid UDP imsi symbols for IMSI: ");
        return false;
    }
    return ;
}