#include "Client.h"

#include <string>

// Точка входа клиентского приложения PGW
// Занимается парсингом аргументов командной строки, запуск Client

int main(int argc, char* argv[]) {
    // Определение пути к конфигурации
    std::string configPath = "configs/pgw_client.json";
    std::string imsi = "000111222333444";

    if(argc > 1) configPath = argv[1];
    if(argc > 2) imsi = argv[2];

    // Создание и запуск приложения
    pgw::client::Client client(configPath,imsi);
    return client.run();
}