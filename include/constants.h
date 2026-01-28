#ifndef PGW_CONSTANTS_H
#define PGW_CONSTANTS_H

#include "types.h"

#include <string_view>

namespace pgw {
namespace constants {

// Неиспользованные константы будут исключены при компиляции средствами оптимизации
namespace client::defaults {
    inline constexpr pgw::types::constIp_t SERVER_IP {"127.0.0.1"};
    inline constexpr pgw::types::port_t SERVER_PORT {9000};
    inline constexpr pgw::types::constFilePath_t LOG_FILE {"client.log"};
    inline constexpr pgw::types::constLogLevel_t LEVEL {"INFO"};
}

namespace server::defaults {
    inline constexpr pgw::types::constIp_t UDP_IP {"0.0.0.0"};
    inline constexpr pgw::types::port_t UDP_PORT {9000};
    inline constexpr pgw::types::port_t HTTP_PORT {8080};

    inline constexpr pgw::types::seconds_t TIMEOUT_SEC {30};
    inline constexpr pgw::types::rate_t GRACEFUL_SHUTDOWN_RATE {10};

    inline constexpr pgw::types::constFilePath_t CDR_FILE {"cdr.log"};
    inline constexpr pgw::types::constFilePath_t LOG_FILE {"pgw.log"};
    inline constexpr pgw::types::constLogLevel_t LEVEL {"INFO"};
}

namespace validation {
    inline constexpr pgw::types::port_t MIN_PORT {1024};
    inline constexpr pgw::types::port_t MAX_PORT {65535};
    inline constexpr pgw::types::seconds_t MIN_SESSION_TIMEOUT {0};
    inline constexpr pgw::types::rate_t MIN_GRACEFUL_SHUTDOWN_RATE {0};
    inline constexpr size_t IMSI_LENGTH{15};
}

} // namespace constants
} // namespace pgw

#endif // PGW_CONSTANTS_H