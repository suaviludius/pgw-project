#ifndef MOCK_TCP_SOCKET_H
#define MOCK_TCP_SOCKET_H

#include "ITcpSocket.h"

#include <gmock/gmock.h>

class MockTcpSocket : public pgw::ITcpSocket {
public:
    MOCK_METHOD(void, bind, (pgw::types::constIp_t ip, pgw::types::port_t port), (override));
    MOCK_METHOD(std::optional<std::unique_ptr<pgw::ITcpSocket>>, accept, (), (override));
    MOCK_METHOD(void, connect, (pgw::types::constIp_t ip, pgw::types::port_t port), (override));
    MOCK_METHOD(void, send, (std::string_view data), (override));
    MOCK_METHOD(ISocket::Packet, receive, (), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(int, getFd, (), (const, override));
    MOCK_METHOD(const sockaddr_in&, getAddr, (), (const, override));
    MOCK_METHOD(bool, isListening, (), (const, override));
};

#endif // MOCK_TCP_SOCKET_H
