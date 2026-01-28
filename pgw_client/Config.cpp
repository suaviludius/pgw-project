#include "constants.h"
#include "Config.h"
#include "validation.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <iostream>

namespace pgw {
namespace client {

Config::Config(const std::string& configPath){
    try{
        readConfigFile(configPath);
        validateConfigData();
        m_verification = true;
    }
    catch(const std::exception& e) {
        std::cerr << std::string("Config initialization failed. ") << e.what() << '\n';
        std::cerr << "Config set default parameters" << '\n';
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
    m_serverIp = jsonConfig.value("server_ip", pgw::constants::client::defaults::SERVER_IP);
    m_serverPort = jsonConfig.value("server_port", pgw::constants::client::defaults::SERVER_PORT);
    m_logFile = jsonConfig.value("log_file", pgw::constants::client::defaults::LOG_FILE);
    m_logLevel = jsonConfig.value("log_level", pgw::constants::client::defaults::LEVEL);
}

void Config::validateConfigData(){
    // Валидация портов
    if(!pgw::validation::is_valid_port(m_serverPort)){
        throw std::runtime_error("Invalid UDP port: " +
              std::to_string(m_serverPort));
    }
}

void Config::setDefaultConfig() {
    m_serverIp = pgw::constants::client::defaults::SERVER_IP;
    m_serverPort = pgw::constants::client::defaults::SERVER_PORT;
    m_logFile = pgw::constants::client::defaults::LOG_FILE;
    m_logLevel = pgw::constants::client::defaults::LEVEL;
}

}   // namespace client
}   // namespace pgw