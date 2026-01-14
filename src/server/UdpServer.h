#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "ISessionManager.h"
#include "ICdrWriter.h"
#include "ISocket.h"

//#include <memory> // unique_ptr

class UdpServer{
private:
    static constexpr uint32_t MAX_DATAGRAM_SIZE {1500};
    // Ассоциация
    ISessionManager& m_sessionManager;
    ICdrWriter& m_cdrWriter;
    // Композиция
    std::unique_ptr<ISocket> m_socket;
    pgw::types::Ip m_ip;
    pgw::types::Port m_port;
    bool m_running;

public:
    // Передаем соккет в конструктор, чтобы тестировать и много не думать
    explicit UdpServer(
        ISessionManager& sessionManager,
        ICdrWriter& m_cdrWriter,
        pgw::types::ConstIp ip,
        pgw::types::Port port
    );
    ~UdpServer();

    void start();
    void stop();
    void handler();

    bool isRunning() const {return m_running;}
    int getFd() const {return m_socket->getFd();}; // Файловый дискриптор для poll
    bool validateImsi(const std::string& imsi);
};


#endif // UDP_SERVER_H