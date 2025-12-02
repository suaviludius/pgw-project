#include <iostream>
#include <nlohmann/json.hpp>

//#include "config.h"

int main(){
    std::cout << "PGW server start ..." << '\n';

    nlohmann::json config = {
        {"udp_port", 9000},
        {"http_port", 8080}
    };

    std::cout << "Port config: " << config["udp_port"] << '\n';
    return 0;
}