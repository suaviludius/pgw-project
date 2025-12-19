#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "SessionManager.h"
#include "CdrWriter.h"
#include "Socket.h"

class UdpServer{
public:
    static constexpr uint32_t MAX_DATAGRAM_SIZE {1500};
private:
    // Ассоциация
    SessionManager& m_sessionManager;
    CdrWriter& m_cdrWriter;

    // Композиция
    Socket m_socket;
    pgw::types::ConstIp m_ip;
    pgw::types::Port m_port;

    bool m_running;

    void run();

    bool validateImsi(const std::string& imsi);
public:
    UdpServer(SessionManager& sessionManager,
              CdrWriter& m_cdrWriter,
              pgw::types::ConstIp ip,
              pgw::types::Port port);
    ~UdpServer();

    void start();
    void stop();
    bool isRunning() const {return m_running;}
};


#endif // UDP_SERVER_H