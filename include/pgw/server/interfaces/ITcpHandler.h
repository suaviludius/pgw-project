#ifndef I_TCP_HANDLER_H
#define I_TCP_HANDLER_H

#include "protocol.h"

namespace pgw {

struct ITcpHandler {

    virtual ~ITcpHandler() = default;

    // Главный автомат-обработчик команд TCP сервера
    virtual protocol::Message handle(const protocol::Message& request) = 0;
};

} // namespace pgw

#endif // I_TCP_HANDLER_H
