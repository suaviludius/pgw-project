#include "config.h"
#include "constants.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <stdexcept>
#include <fstream>


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
    m_udpIp = jsonConfig.value("udp_ip", pgw::constants::defaults::UDP_IP); // UdpIp - имя ключа, 0.0.0.0 - значение по умолчанию
    m_udpPort = jsonConfig.value("udp_port", pgw::constants::defaults::UDP_PORT);
    m_sessionTimeoutSec = jsonConfig.value("session_timeout_sec", pgw::constants::defaults::TIMEOUT_SEC);
    m_cdrFile = jsonConfig.value("cdr_file",pgw::constants::defaults::CDR_FILE);
    m_httpPort = jsonConfig.value("http_port",pgw::constants::defaults::HTTP_PORT);
    m_gracefulShutdownRate = jsonConfig.value("graceful_shutdown_rate",pgw::constants::defaults::GRACEFUL_SHUTDOWN_RATE);
    m_logFile = jsonConfig.value("log_file",pgw::constants::defaults::LOG_FILE);
    m_logLevel = jsonConfig.value("log_level",pgw::constants::defaults::LEVEL);

    if(jsonConfig.contains("blacklist") && jsonConfig["blacklist"].is_array()){
        for (const auto& imsi : jsonConfig["blacklist"]) { // Проходим по всем JSON ОБЪЕКТАМ (не строкам) из "blacklist"
            m_blackList.add(imsi.get<pgw::types::ConstImsi>());
        }
    }
}

void Config::validateConfigData(){

    // Валидация портов
    if (m_udpPort == pgw::constants::validation::MIN_PORT ||
        m_udpPort > pgw::constants::validation::MAX_PORT) {
        throw std::runtime_error("Invalid UDP port: " +
              std::to_string(m_udpPort));
    }

    if (m_httpPort == pgw::constants::validation::MIN_PORT ||
        m_httpPort > pgw::constants::validation::MAX_PORT) {
        throw std::runtime_error("Invalid HTTP port: " +
              std::to_string(m_httpPort));
    }

    // Валидация таймаутов и лимитов
    if (m_sessionTimeoutSec <= pgw::constants::validation::MIN_SESSION_TIMEOUT) {
        throw std::runtime_error("Session timeout must be greater than " +
              std::to_string(pgw::constants::validation::MIN_SESSION_TIMEOUT) +
              " seconds");
    }

    if (m_gracefulShutdownRate <= pgw::constants::validation::MIN_GRACEFUL_SHUTDOWN_RATE) {
        throw std::runtime_error("Graceful shutdown rate must be greater than " +
              std::to_string(pgw::constants::validation::MIN_GRACEFUL_SHUTDOWN_RATE) +
              " seconds");
    }

    // Валидация IMSI в blackList
    for (const auto& imsi : m_blackList) {
        if (imsi.size() != pgw::constants::validation::IMSI_SIZE) {
            throw std::runtime_error("Invalid IMSI in blacklist: " + static_cast<std::string>(imsi));
        }
    }
}

void Config::setDefaultConfig() {
    m_udpIp = pgw::constants::defaults::UDP_IP;
    m_udpPort = pgw::constants::defaults::UDP_PORT;
    m_sessionTimeoutSec = pgw::constants::defaults::TIMEOUT_SEC;
    m_cdrFile = pgw::constants::defaults::CDR_FILE;
    m_httpPort = pgw::constants::defaults::HTTP_PORT;
    m_gracefulShutdownRate = pgw::constants::defaults::GRACEFUL_SHUTDOWN_RATE;
    m_logFile = pgw::constants::defaults::LOG_FILE;
    m_logLevel = pgw::constants::defaults::LEVEL;
    m_blackList.clear();
}
