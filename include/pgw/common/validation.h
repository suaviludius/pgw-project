#ifndef PGW_VALIDATION
#define PGW_VALIDATION

#include "constants.h"
#include "types.h"

#include <algorithm> // all_of()

namespace pgw{
namespace validation{

inline bool isValidPort(pgw::types::port_t port) {
    return port > pgw::constants::validation::MIN_PORT &&
           port <= pgw::constants::validation::MAX_PORT;
}

inline bool isValidImsi(pgw::types::constImsi_t imsi) {
    if (imsi.length() != pgw::constants::validation::IMSI_LENGTH) {
        return false;
    }
    if (!std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        return false;
    }
    return true;
}

inline bool isValidSessionTimeout(pgw::types::seconds_t timeout) {
    return timeout > constants::validation::MIN_SESSION_TIMEOUT;
}

inline bool isValidGracefulShutdownRate(pgw::types::rate_t rate) {
    return rate > constants::validation::MIN_GRACEFUL_SHUTDOWN_RATE;
}

inline bool isValidBlacklist(const pgw::types::Blacklist& blacklist) {
    for (const auto& imsi : blacklist) {
        if (!isValidImsi(imsi)) {
            return false;
        }
    }
    return true;
}

} // namespace pgw
} // namespace validation

#endif //PGW_VALIDATION
