#ifndef MOCK_SOCKET_H
#define MOCK_SOCKET_H

#include "ISocket.h"

#include <gmock/gmock.h>

class MockSocket : public ISocket {
public:
    // MockSocket() = default;
    // virtual ~MockSocket() = default;
    MOCK_METHOD(void, bind, (pgw::types::constIp_t ip, pgw::types::port_t port), (override));
    MOCK_METHOD(void, send, (std::string_view data, const sockaddr_in& address), (override));
    MOCK_METHOD(void, send, (std::string_view data, pgw::types::constIp_t ip, pgw::types::port_t port), (override));
    MOCK_METHOD(Packet, receive, (), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(int, getFd, (), (const override));
    MOCK_METHOD(const sockaddr_in&, getAddr, (), (const override));
    MOCK_METHOD(std::string, addrToString, (const sockaddr_in& addr), (override));
};

#endif // MOCK_SOCKET_H