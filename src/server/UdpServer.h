#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "SessionManager.h"
#include "CdrWriter.h"

#include <windows.h>

// Для Unix систем
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>

class UdpServer{
    SessionManager& m_sessionManager;
    CdrWriter& m_cdrWriter;
    bool m_running;

    bool createSocket();
    bool run();
    bool sendResponse();
public:
    UdpServer(SessionManager& sessionManager, CdrWriter& m_cdrWriter, pgw::types::ConstIp ip);
    ~UdpServer();
    void start();
    void stop();
    bool isRunning() const;
};


#endif // UDP_SERVER_H