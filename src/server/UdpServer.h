#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "SessionManager.h"
#include "CdrWriter.h"

#//include <windows.h>

// Для Unix систем
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class UdpServer{
public:
    static constexpr uint32_t MAX_DATAGRAM_SIZE {1500};
private:
    SessionManager& m_sessionManager;
    CdrWriter& m_cdrWriter;

    // Сокет
    in_addr m_socketIp; // Хранит 32-битный IP-адрес хоста
    sockaddr_in m_socketIPv4; // Структура для сокета IPv4 (Адрес + порт + ...)
    sockaddr* m_socket; // Универсальная струтура для всех типов сокетов
    int m_socketFd; // Файловый дискриптор сокета

    bool m_running;

    bool createSocket();
    bool bindSocket();
    bool run();
    bool sendResponse();

    std::string binaryToSTring(const char* buffer, size_t length);
public:
    UdpServer(SessionManager& sessionManager,
              CdrWriter& m_cdrWriter,
              pgw::types::ConstIp ip,
              pgw::types::Port port);
    ~UdpServer();
    bool start();
    bool stop();
    bool isRunning() const;
};


#endif // UDP_SERVER_H