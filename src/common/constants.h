#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "types.h"
#include <string_view>

namespace constants {
    namespace defaults {
        inline constexpr std::string_view UDP_IP {"0.0.0.0"};
        inline constexpr types::Port UDP_PORT {9000};
        inline constexpr types::Port HTTP_PORT {8080};

        inline constexpr types::Seconds TIMEOUT_SEC {30};
        inline constexpr types::Rate GRACEFUL_SHUTDOWN_RATE {10};

        inline constexpr std::string_view CDR_FILE {"cdr.log"};
        inline constexpr std::string_view LOG_FILE {"pgw.log"};
        inline constexpr std::string_view LEVEL {"INFO"};
    }
    namespace validation {
        inline constexpr types::Port  MIN_PORT = 1024;
        inline constexpr types::Port MAX_PORT = 65535;
        inline constexpr types::Seconds MIN_SESSION_TIMEOUT = 0;
        inline constexpr types::Rate MIN_GRACEFUL_SHUTDOWN_RATE = 0;
        inline constexpr size_t IMSI_SIZE{15};
    }
}

#endif // CONSTANTS_H