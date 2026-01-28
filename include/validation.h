#ifndef PGW_VALIDATION
#define PGW_VALIDATION

#include "constants.h"
#include "types.h"

#include <algorithm> // all_of()

namespace pgw{
namespace validation{

inline bool is_valid_port(pgw::types::port_t port) {
    return port > pgw::constants::validation::MIN_PORT &&
           port <= pgw::constants::validation::MAX_PORT;
}

inline bool is_valid_imsi(pgw::types::constImsi_t imsi) {
    if (imsi.length() != pgw::constants::validation::IMSI_LENGTH) {
        return false;
    }
    if (!std::all_of(imsi.begin(), imsi.end(), ::isdigit)) {
        return false;
    }
    return true;
}

inline bool is_valid_session_timeout(pgw::types::seconds_t timeout) {
    return timeout > constants::validation::MIN_SESSION_TIMEOUT;
}

inline bool is_valid_graceful_shutdown_rate(pgw::types::rate_t rate) {
    return rate > constants::validation::MIN_GRACEFUL_SHUTDOWN_RATE;
}

inline bool is_valid_blacklist(const pgw::types::Blacklist& blacklist) {
    for (const auto& imsi : blacklist) {
        if (!is_valid_imsi(imsi)) {
            return false;
        }
    }
    return true;
}

} // namespace pgw
} // namespace validation

#endif //PGW_VALIDATION