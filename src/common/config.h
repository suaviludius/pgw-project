// Общая конфигурация файла
#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

class Config{
    pgw::types::Ip m_udpIp{};
    pgw::types::Port m_udpPort{};
    pgw::types::Seconds m_sessionTimeoutSec{};
    pgw::types::FilePath m_cdrFile {};
    pgw::types::Port m_httpPort{};
    pgw::types::Rate m_gracefulShutdownRate{};
    pgw::types::FilePath m_logFile {};
    pgw::types::FilePath m_logLevel {};
    pgw::types::Blacklist m_blackList;

    std::string m_error{};
    bool m_verification{true};

    void readConfigFile(const std::string& confPath);
    void validateConfigData();
    void setDefaultConfig();

public:
    explicit Config(const std::string& configPath); // Требуем вызов конструктора только с явным типом аргумента

    const pgw::types::Ip& getUdpIp() const { return m_udpIp; }
    pgw::types::Port getUdpPort() const { return m_udpPort; }
    pgw::types::Seconds getSessionTimeoutSec() const {return m_sessionTimeoutSec; }
    const pgw::types::FilePath& getCdrFile() const { return m_cdrFile; }
    pgw::types::Port getHttpPort() const { return m_httpPort; }
    pgw::types::Rate getGracefulShutdownRate() const { return m_gracefulShutdownRate; }
    const pgw::types::FilePath& getLogFile() const { return m_logFile; }
    const pgw::types::LogLevel& getLogLevel() const { return m_logLevel; }
    const pgw::types::Blacklist& getBlacklist() const { return m_blackList; }
};

#endif // CONFIG_H