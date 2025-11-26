// Общая конфигурация файла
#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

class Config{
    types::Ip m_udpIp{};
    types::Port m_udpPort{};
    types::Seconds m_sessionTimeoutSec{};
    types::FilePath m_cdrFile {};
    types::Port m_httpPort{};
    types::Rate m_gracefulShutdownRate{};
    types::FilePath m_logFile {};
    types::FilePath m_logLevel {};
    types::Blacklist m_blackList;

    std::string m_error{};
    bool m_verification{true};

    void readConfigFile(const std::string& confPath);
    void validateConfigData();
    void setDefaultConfig();

public:
    explicit Config(const std::string& configPath); // Требуем вызов конструктора только с явным типом аргумента

    const types::Ip& getUdpIp() const { return m_udpIp; }
    types::Port getUdpPort() const { return m_udpPort; }
    types::Seconds getSessionTimeoutSec() const {return m_sessionTimeoutSec; }
    const types::FilePath& getCdrFile() const { return m_cdrFile; }
    types::Port getHttpPort() const { return m_httpPort; }
    types::Rate getGracefulShutdownRate() const { return m_gracefulShutdownRate; }
    const types::FilePath& getLogFile() const { return m_logFile; }
    const types::LogLevel& getLogLevel() const { return m_logLevel; }
    const types::Blacklist& getBlacklist() const { return m_blackList; }
};

#endif // CONFIG_H