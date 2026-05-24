#ifndef MOCK_TCP_HANDLER_H
#define MOCK_TCP_HANDLER_H

#include "ITcpHandler.h"

#include <gmock/gmock.h>

class MockTcpHandler : public pgw::ITcpHandler {
public:
    MOCK_METHOD(pgw::protocol::Message, handle, (const pgw::protocol::Message& request), (override));
};

#endif // MOCK_TCP_HANDLER_H

// TODO: стоит перенести все моки в пространство pgw::