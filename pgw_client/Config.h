// Общая конфигурация файла
#ifndef PGW_CLIENT_CONFIG_H
#define PGW_CLIENT_CONFIG_H

#include "types.h"

namespace pgw {
namespace client {

class Config{
    pgw::types::ip_t m_serverIp{};
    pgw::types::port_t m_serverPort{};
    pgw::types::filePath_t m_logFile {};
    pgw::types::logLevel_t m_logLevel {};

    bool m_verification{true};

    void readConfigFile(const std::string& confPath);
    void validateConfigData();
    void setDefaultConfig();

public:
    explicit Config(const std::string& configPath);

    pgw::types::constIp_t getServerIp() const { return m_serverIp; }
    pgw::types::port_t getServerPort() const { return m_serverPort; }
    pgw::types::constFilePath_t getLogFile() const { return m_logFile; }
    pgw::types::constLogLevel_t getLogLevel() const { return m_logLevel; }

    bool isValid() const { return m_verification; }
};

}   // namespace client
}   // namespace pgw

#endif // PGW_CLIENT_CONFIG_H