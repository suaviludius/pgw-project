#ifndef PGW_UDP_SERVER_H
#define PGW_UDP_SERVER_H

#include "ISessionManager.h"
#include "ICdrWriter.h"
#include "IUdpSocket.h"

#include <memory>

namespace pgw {

class UdpServer{
private:
    static constexpr uint32_t MAX_DATAGRAM_SIZE = 1500;

    ISessionManager& m_sessionManager;
    std::unique_ptr<IUdpSocket> m_socket;

    pgw::types::ip_t m_ip;
    pgw::types::port_t m_port;

    bool m_running;

public:
    explicit UdpServer(
        ISessionManager& sessionManager,
        pgw::types::constIp_t ip,
        pgw::types::port_t port,
        std::unique_ptr<IUdpSocket> socket = nullptr
    );
    ~UdpServer();

    void start();
    void stop();
    void handler();

    bool isRunning() const { return m_running; }
    int getFd() const { return m_socket->getFd(); }

    bool validateImsi(const std::string& imsi);
};

} // namespace pgw

#endif // PGW_UDP_SERVER_H
