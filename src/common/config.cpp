#ifndef CONFIG_CPP
#define CONFIG_CPP

#include "config.h"
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
    }
}

void readConfigFile(const std::string& configPath){
    std::ifstream file(configPath);
    if(!file.is_open()){
        throw std::runtime_error("Connot open config file: " + configPath);
    }

    nlohmann::json jsonConfig;
    file >> jsonConfig;
    // Инициализируем поля конфигурации значениями из файла
    // . . .
}

void validateConfigData(){
    // Нужно проверить соответсвтуют ли данные из файла нужному формату
    // . . .
}

#endif // CONFIG_CPP