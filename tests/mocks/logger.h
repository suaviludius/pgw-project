#ifndef LOGGER_H
#define LOGGER_H

#include <string_view>
#include <iostream>

class Logger {
public:
    static void init(std::string_view, std::string_view) {std::cout << "[Logger] Initialized\n";}
    static void shutdown() {std::cout << "[Logger] Shutdown\n";}

    static void trace(std::string_view msg) {std::cout << "[TRACE] " << msg << "\n";}
    static void debug(std::string_view msg) {std::cout << "[DEBUG] " << msg << "\n";}
    static void info(std::string_view msg) {std::cout << "[INFO] " << msg << "\n";}
    static void warn(std::string_view msg) {std::cout << "[WARN] " << msg << "\n";}
    static void error(std::string_view msg) {std::cout << "[ERROR] " << msg << "\n";}
    static void critical(std::string_view msg) {std::cout << "[CRITICAL] " << msg << "\n";}

    static void session_created(std::string_view imsi) {std::cout << "[SESSION_CREATED] " << imsi << "\n";}
    static void session_rejected(std::string_view imsi, std::string_view reason) {std::cout << "[SESSION_REJECTED] " << imsi << " reason: " << reason << "\n";}
    static void session_deleted(std::string_view imsi) {std::cout << "[SESSION_DELETED] " << imsi << "\n";}
    static void udp_request(std::string_view, std::string_view) {}
    static void http_request(std::string_view, std::string_view) {}
};
#endif // LOGGER_H