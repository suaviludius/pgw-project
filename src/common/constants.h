#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "types.h"
#include <string_view>

namespace pgw::constants {
    namespace defaults {
        inline constexpr pgw::types::ConstIp UDP_IP {"0.0.0.0"};
        inline constexpr pgw::types::Port UDP_PORT {9000};
        inline constexpr pgw::types::Port HTTP_PORT {8080};

        inline constexpr pgw::types::Seconds TIMEOUT_SEC {30};
        inline constexpr pgw::types::Rate GRACEFUL_SHUTDOWN_RATE {10};

        inline constexpr pgw::types::ConstFilePath CDR_FILE {"cdr.log"};
        inline constexpr pgw::types::ConstFilePath LOG_FILE {"pgw.log"};
        inline constexpr pgw::types::ConstLogLevel LEVEL {"INFO"};
    }
    namespace validation {
        inline constexpr pgw::types::Port MIN_PORT {1024};
        inline constexpr pgw::types::Port MAX_PORT {65535};
        inline constexpr pgw::types::Seconds MIN_SESSION_TIMEOUT {0};
        inline constexpr pgw::types::Rate MIN_GRACEFUL_SHUTDOWN_RATE {0};
        inline constexpr size_t IMSI_SIZE{15};
    }
}

#endif // CONSTANTS_H