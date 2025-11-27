#include "config.h"
#include "constants.h"

#include <stdexcept>
#include <fstream>
#include <nlohmann/json.hpp>

Config::Config(const std::string& configPath){
    try{
        readConfigFile(configPath);
        validateConfigData();
        m_verification = true;
    }
    catch(const std::exception& e) {
        m_error = std::string("Config initialization failed: ") + e.what();
        m_verification = false;
        setDefaultConfig();
    }
}

void Config::readConfigFile(const std::string& configPath){
    std::ifstream file(configPath);
    if(!file.is_open()){
        throw std::runtime_error("Connot open config file: " + configPath);
    }

    nlohmann::json jsonConfig; // Создаем пустой JSON-объект
    jsonConfig = nlohmann::json::parse(file); // !!! Может выдать исключение nlohmann::json::exception
    file.close();
    // Присваиваем значения полям конфигурации значениями из файла
    m_udpIp = jsonConfig.value("udp_ip", constants::defaults::UDP_IP); // UdpIp - имя ключа, 0.0.0.0 - значение по умолчанию
    m_udpPort = jsonConfig.value("udp_port", constants::defaults::UDP_PORT);
    m_sessionTimeoutSec = jsonConfig.value("session_timeout_sec", constants::defaults::TIMEOUT_SEC);
    m_cdrFile = jsonConfig.value("cdr_file",constants::defaults::CDR_FILE);
    m_httpPort = jsonConfig.value("http_port",constants::defaults::HTTP_PORT);
    m_gracefulShutdownRate = jsonConfig.value("graceful_shutdown_rate",constants::defaults::GRACEFUL_SHUTDOWN_RATE);
    m_logFile = jsonConfig.value("log_file",constants::defaults::LOG_FILE);
    m_logLevel = jsonConfig.value("log_level",constants::defaults::LEVEL);

    if(jsonConfig.contains("blacklist") && jsonConfig["blacklist"].is_array()){
        for (const auto& item : jsonConfig["numbers"]) {
            m_blackList.push_back(item);
        }
    }
}

void Config::validateConfigData(){

    // Валидация портов
    if (m_udpPort == constants::validation::MIN_PORT ||
        m_udpPort > constants::validation::MAX_PORT) {
        throw std::runtime_error("Invalid UDP port: " +
              std::to_string(m_udpPort));
    }

    if (m_httpPort == constants::validation::MIN_PORT ||
        m_httpPort > constants::validation::MAX_PORT) {
        throw std::runtime_error("Invalid HTTP port: " +
              std::to_string(m_httpPort));
    }

    // Валидация таймаутов и лимитов
    if (m_sessionTimeoutSec <= constants::validation::MIN_SESSION_TIMEOUT) {
        throw std::runtime_error("Session timeout must be greater than " +
              std::to_string(constants::validation::MIN_SESSION_TIMEOUT) +
              " seconds");
    }

    if (m_gracefulShutdownRate <= constants::validation::MIN_GRACEFUL_SHUTDOWN_RATE) {
        throw std::runtime_error("Graceful shutdown rate must be greater than " +
              std::to_string(constants::validation::MIN_GRACEFUL_SHUTDOWN_RATE) +
              " seconds");
    }

    // Валидация IMSI в blackList
    for (const auto& imsi : m_blackList) {
        if (imsi.size() != constants::validation::IMSI_SIZE) {
            throw std::runtime_error("Invalid IMSI in blacklist: " + imsi);
        }
    }
}

void Config::setDefaultConfig() {
    m_udpIp = constants::defaults::UDP_IP;
    m_udpPort = constants::defaults::UDP_PORT;
    m_sessionTimeoutSec = constants::defaults::TIMEOUT_SEC;
    m_cdrFile = constants::defaults::CDR_FILE;
    m_httpPort = constants::defaults::HTTP_PORT;
    m_gracefulShutdownRate = constants::defaults::GRACEFUL_SHUTDOWN_RATE;
    m_logFile = constants::defaults::LOG_FILE;
    m_logLevel = constants::defaults::LEVEL;
    m_blackList.clear();
}