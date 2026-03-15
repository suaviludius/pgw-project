// Общая конфигурация файла
#ifndef PGW_SERVER_CONFIG_H
#define PGW_SERVER_CONFIG_H

#include "types.h"

namespace pgw {
namespace server {

class Config{
    pgw::types::ip_t m_udpIp{};
    pgw::types::port_t m_udpPort{};
    pgw::types::seconds_t m_sessionTimeoutSec{};
    pgw::types::filePath_t m_cdrFile {};
    pgw::types::port_t m_httpPort{};
    pgw::types::rate_t m_gracefulShutdownRate{};
    pgw::types::filePath_t m_logFile {};
    pgw::types::filePath_t m_logLevel {};
    pgw::types::Blacklist m_blackList;

    std::string m_error{};
    bool m_verification{true};

    void readConfigFile(const std::string& confPath);
    void validateConfigData();
    void setDefaultConfig();

public:
    explicit Config(const std::string& configPath); // Требуем вызов конструктора только с явным типом аргумента

    pgw::types::constIp_t getUdpIp() const { return m_udpIp; }
    pgw::types::port_t getUdpPort() const { return m_udpPort; }
    pgw::types::seconds_t getSessionTimeoutSec() const {return m_sessionTimeoutSec; }
    pgw::types::constFilePath_t getCdrFile() const { return m_cdrFile; }
    pgw::types::port_t getHttpPort() const { return m_httpPort; }
    pgw::types::rate_t getGracefulShutdownRate() const { return m_gracefulShutdownRate; }
    pgw::types::constFilePath_t getLogFile() const { return m_logFile; }
    pgw::types::constLogLevel_t getLogLevel() const { return m_logLevel; }
    auto& getBlacklist() { return m_blackList; } // Длинный тип, поэтому auto

    std::string_view const getError(){ return m_error;}
    bool isValid(){ return m_verification;}
};

}   // namespace server
}   // namespace pgw

#endif // SERVER_CONFIG_H