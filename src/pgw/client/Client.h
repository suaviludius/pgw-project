#ifndef PGW_CLIENT_H
#define PGW_CLIENT_H

#include "Config.h"
#include "IUdpSocket.h"

#include <memory>

namespace pgw {
namespace client {

class Client{
private:
    // Конфигурация приложения (создается один раз в конструкторе)
    std::unique_ptr<Config> m_config;

    // Imsi клиента-абонента (создается один раз в конструкторе)
    pgw::types::imsi_t m_imsi;

    // Соккет для ообщения (создается один раз в конструкторе)
    std::unique_ptr<IUdpSocket> m_socket;

    // Основной цикл обработки событий
    void runEventLoop();

    // Инициализатор логгера
    bool initializeLogger();

public:
    explicit Client(const pgw::types::filePath_t& configPath, const pgw::types::imsi_t& imsi);
    ~Client();

    // Запрещаем копирование и перемещение (Правило пяти)
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;

    // Запускает приложение (основной цикл обработки событий)
    int run();

};

} // namespace client
} // namespace pgw

#endif // PGW_CLIENT_H
