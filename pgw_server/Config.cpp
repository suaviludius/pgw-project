#include "constants.h"
#include "Config.h"
#include "validation.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <stdexcept>
#include <fstream>

namespace pgw {
namespace server {

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
    m_udpIp = jsonConfig.value("udp_ip", pgw::constants::server::defaults::UDP_IP); // UdpIp - имя ключа, 0.0.0.0 - значение по умолчанию
    m_udpPort = jsonConfig.value("udp_port", pgw::constants::server::defaults::UDP_PORT);
    m_sessionTimeoutSec = jsonConfig.value("session_timeout_sec", pgw::constants::server::defaults::TIMEOUT_SEC);
    m_cdrFile = jsonConfig.value("cdr_file",pgw::constants::server::defaults::CDR_FILE);
    m_httpPort = jsonConfig.value("http_port",pgw::constants::server::defaults::HTTP_PORT);
    m_gracefulShutdownRate = jsonConfig.value("graceful_shutdown_rate",pgw::constants::server::defaults::GRACEFUL_SHUTDOWN_RATE);
    m_logFile = jsonConfig.value("log_file",pgw::constants::server::defaults::LOG_FILE);
    m_logLevel = jsonConfig.value("log_level",pgw::constants::server::defaults::LEVEL);

    if(jsonConfig.contains("blacklist") && jsonConfig["blacklist"].is_array()){
        for (const auto& imsi : jsonConfig["blacklist"]) { // Проходим по всем JSON ОБЪЕКТАМ (не строкам) из "blacklist"
            m_blackList.add(imsi.get<pgw::types::imsi_t>());
        }
    }
}

void Config::validateConfigData(){

    // Валидация портов
    if (!pgw::validation::is_valid_port(m_udpPort)) {
        throw std::runtime_error("Invalid UDP port: " +
              std::to_string(m_udpPort));
    }

    if (!pgw::validation::is_valid_port(m_httpPort)) {
        throw std::runtime_error("Invalid HTTP port: " +
              std::to_string(m_httpPort));
    }

    if (!pgw::validation::is_valid_session_timeout(m_sessionTimeoutSec)) {
        throw std::runtime_error("Invalid session timeout: " +
              std::to_string(m_sessionTimeoutSec));
    }

    if (!pgw::validation::is_valid_graceful_shutdown_rate(m_gracefulShutdownRate)) {
        throw std::runtime_error("Invalid shutdown rate: " +
              std::to_string(m_gracefulShutdownRate));
    }

    // Валидация IMSI в blackList
    if (!pgw::validation::is_valid_blacklist(m_blackList)) {
        throw std::runtime_error("Invalid IMSI in blacklist");
    }
}

void Config::setDefaultConfig() {
    m_udpIp = pgw::constants::server::defaults::UDP_IP;
    m_udpPort = pgw::constants::server::defaults::UDP_PORT;
    m_sessionTimeoutSec = pgw::constants::server::defaults::TIMEOUT_SEC;
    m_cdrFile = pgw::constants::server::defaults::CDR_FILE;
    m_httpPort = pgw::constants::server::defaults::HTTP_PORT;
    m_gracefulShutdownRate = pgw::constants::server::defaults::GRACEFUL_SHUTDOWN_RATE;
    m_logFile = pgw::constants::server::defaults::LOG_FILE;
    m_logLevel = pgw::constants::server::defaults::LEVEL;
    m_blackList.clear();
}

}   // namespace server
}   // namespace pgw