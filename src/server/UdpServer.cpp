#include "UdpServer.h"
#include "logger.h"

UdpServer::UdpServer(SessionManager& sessionManager, CdrWriter& cdrWriter, pgw::types::ConstIp ip)
    : m_sessionManager{sessionManager}, m_cdrWriter{cdrWriter} {
    int fd, fd2; /* дескрипторы */
    struct sockaddr_in server; // Информация
    if (inet_pton(AF_INET, static_cast<std::string>(ip), &local_ip) != 1) {
        throw std::runtime_error("Invalid UDP IP address for server");
    }
}

UdpServer::~UdpServer(){
    stop();
}

void UdpServer::start(){
    // . . .
}


void UdpServer::stop(){
    // . . .
}


bool UdpServer::isRunning() const{
    // . . .
}


bool UdpServer::createSocket(){
    // . . .
}


bool UdpServer::run(){
    // . . .
}