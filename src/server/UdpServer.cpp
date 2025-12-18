#include "UdpServer.h"
#include "logger.h"
#include <string.h> // memset
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
        m_socket.bind(m_ip,m_port);
        m_running = true;
        Logger::info("UDP server sucseccdully start");
        // run()
    }
    catch(const std::exception& e){
        m_running = false;
    }
}

void UdpServer::stop(){
    if(!m_running) return;
    m_running = false;
    Logger::info("UDP server stopped");
}


bool UdpServer::run(){
    while(m_running){
        try{
            auto packet = m_socket.recieve();
            if(!packet.data.empty()){
                // . . .
            }
        }
        catch (const std::system_error& e) {
            // . . .
        }
        catch(const std::exception& e){
            Logger::error("UDP server error: " + std::string(e.what()));
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