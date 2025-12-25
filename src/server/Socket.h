#ifndef SOCKET_H
#define SOCKET_H

#include "ISocket.h"

class Socket : public ISocket {
private:
    static constexpr uint16_t MAX_BYTES_RECIEVE {1024};
    int m_fd; // file diskriptor

public:
    Socket();
    ~Socket() override;

    void bind(pgw::types::ConstIp ip, pgw::types::Port port) override;
    bool send(std::string_view data, const sockaddr_in& addres) override;
    Packet receive() override;
    void close() override;

    int getFd() const override {return m_fd;}
    std::string addrToString(const sockaddr_in& addr) override; // Из sockaddr_in делает string c ip:port
};

#endif // SOCKET_H