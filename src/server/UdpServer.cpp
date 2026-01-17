#include "Socket.h" // Чтобы создать объект Socket()
#include "UdpServer.h"
#include "logger.h"

#include <algorithm> // all_of()
#include <cstring>

UdpServer::UdpServer(ISessionManager& sessionManager,
                     ICdrWriter& cdrWriter,
                     pgw::types::ConstIp ip,
                     pgw::types::Port port,
                     std::unique_ptr<ISocket> socket)
try : m_sessionManager{sessionManager},
      m_cdrWriter{cdrWriter},
      m_socket{socket ? std::move(socket) : std::make_unique<Socket>()}, // Если прийдет пустой указатель, то создаем соккет сами, иначе перемещаем
      // m_socket{std::move(socket)},
      m_ip{ip},
      m_port{port},
      m_running{false}{
    LOG_INFO("UDP server initialized");
}
catch(const std::exception& e){
    LOG_ERROR("UDP server initialization  filed: {}", e.what());
    // Пробрасываем исключение дальше
    throw std::runtime_error("UDP server initialization failed");
}

UdpServer::~UdpServer(){
    stop();
    LOG_INFO("UDP server deleted");
}

void UdpServer::start(){
    if(m_running) {
        LOG_WARN("UDP server already running");
        return;
    }
    try{
        LOG_INFO("Starting UDP server ...");
        m_socket->bind(m_ip,m_port);
        m_running = true;
    }
    catch (const std::exception& e){
        LOG_ERROR("UDP server start failed {} : {} - {}", m_ip, m_port, e.what());
        // Пробрасываем с дополнительным контекстом
        throw std::runtime_error("UDP server start failed");
    }
    LOG_INFO("UDP server started");
    // run();
}

void UdpServer::stop(){
    if(!m_running) {
        LOG_INFO("UDP server already stopped");
        return;
    }
    m_running = false;
    LOG_INFO("UDP server stopped");
}

// Метод раcсчитан на использование с менеджером poll, epoll, select
void UdpServer::handler(){
    if(!m_running) {
        LOG_INFO("UDP server already running");
        return;
    }
    try{
        // Читаем все доступные пакеты
        while(true){
            auto packet = m_socket->receive();
            // Чтение происходит в неблокирующем режиме, т.е. в случае если на сокете данных,
            // то соккет не будет блокировать программный процесс на себе до прихода данных,
            // а просто вернет управление вызывающей функции со словами "не сегодня, дружочек, в другой раз"
            if(packet.data.empty()) break;
            std::string answer {"rejected"};
            std::string imsi = packet.data;
            // Валидация
            if (validateImsi(imsi)) {
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
            }
            m_socket->send(answer,packet.senderAddr);
        }
    }
    catch(const std::exception& e){
        LOG_ERROR("UDP server runninig error: {}", e.what());
    }
}

bool UdpServer::validateImsi(const std::string& imsi){
    if (imsi.length() != 15) {
        LOG_WARN("UDP server receive imsi whith invalid size: {} ", imsi);
        return false;
    }
    if (!std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        LOG_WARN("UDP server receive imsi whith invalid symbols: {}", imsi);
        return false;
    }
    return true;
}