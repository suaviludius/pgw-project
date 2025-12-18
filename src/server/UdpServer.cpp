#include "UdpServer.h"
#include "logger.h"
#include <string.h> // memset
#include <cstring>


UdpServer::UdpServer(SessionManager& sessionManager,
                     CdrWriter& cdrWriter,
                     pgw::types::ConstIp ip,
                     pgw::types::Port port)
    : m_sessionManager{sessionManager},
      m_cdrWriter{cdrWriter} {

    if (inet_pton(AF_INET, ip.c_str(), &m_socketIp) != 1) { // Заносим адресс в переменную
        throw std::runtime_error("Invalid UDP IP address for server");
    }
    memset(&m_socketIPv4, 0, sizeof(m_socketIPv4)); // Заполняем структуру нулями
    m_socketIPv4.sin_addr = m_socketIp; // IP
    m_socketIPv4.sin_family = AF_INET; // IPv4
    m_socketIPv4.sin_port = htons(port); // Преобразует порядок байт в сетевой (big-endian)
    m_socket = reinterpret_cast<struct sockaddr* > (&m_socketIPv4); // Для работ с функциями
}

UdpServer::~UdpServer(){
    stop();
}

bool UdpServer::start(){
    if(m_running) {
        Logger::warn("UDP server already running");
        return true;
    }
    if(!createSocket()){
        Logger::warn("UDP socket create failed");
        return false;
    }
    m_running = true;
    Logger::info("UDP server started");
    return true;
}


void UdpServer::stop(){
    // . . .
}


bool UdpServer::isRunning() const{
    // . . .
}


bool UdpServer::createSocket(){
    // Создаем дискриптор для сокета IPv4 + UDP
    m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socketFd < 0) {
        Logger::error("Filed socket()");
        return false;
    }
    // Включаем повторное использование порта
    int reuse = 1;
    if (setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        Logger::error("Filed setsockopt() for reusing port");
    }
    run();
    return true;
}


bool UdpServer::run(){
    char buffer[MAX_DATAGRAM_SIZE];
    while(1){
        sockaddr_in clientSocketIPv4{};
        socklen_t clientSocketLen = sizeof(sockaddr_in);
        sockaddr* clientSocket = reinterpret_cast<struct sockaddr*>(&clientSocketIPv4);

        ssize_t bufferCount = recvfrom(m_socketFd, buffer, sizeof(buffer),0,clientSocket, &clientSocketLen);
        if (bufferCount < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // Таймаут или прерывание - продолжаем цикл
                continue;
            }
            Logger::error("Failed recvfrom()" + strerror(errno));
            break;
        }

        pgw::types::ConstImsi imsi { getBufferImsi(buffer, bufferCount) };
        if(imsi.empty()){
            Logger::warn("Invalid UDP imsi");
        }
        auto result = m_sessionManager.createSession(imsi);
        switch(result){
            case::SessionManager::CreateResult::ALREADY_EXISTS:
                CdrWriter();
        }
    }
}

pgw::types::ConstImsi UdpServer::getBufferImsi(const char* buffer, size_t length){
    std::string imsi = binaryToSTring(buffer, length);
    if (imsi.empty() || imsi.length() > 15) {
        Logger::warn("Invalid UDP imsi size");
        // Можно отправить error response, но спецификация не требует
        return;
    }
    if (!std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        Logger::warn("Invalid UDP imsi symbols");
        return;
    }
    return imsi;
}

std::string UdpServer::binaryToSTring(const char* buffer, size_t length) {
    std::string result;

    // Один байт = 2 х 16 ричных числа = 2 х 4 бита
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte { buffer[i] };

        // Первая цифра (старшие 4 бита)
        int firstDigit { (byte >> 4) & 0x0F };
        if (firstDigit <= 9) {
            result.push_back('0' + firstDigit);
        }

        // Вторая цифра (младшие 4 бита)
        int secondDigit = byte & 0x0F;
        if (secondDigit <= 9) {
            result.push_back('0' + secondDigit);
        }
    }

    return result;
}