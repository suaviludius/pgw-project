#include "Server.h"

// Точка входа приложения PGW
// Занимается парсингом аргументов командной строки, запуск Application

int main(int argc, char* argv[]) {
    // Определение пути к конфигурации
    std::string configPath = "configs/pgw_server.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    // Создание и запуск приложения
    pgw::server::Server server(configPath);
    return server.run();
}