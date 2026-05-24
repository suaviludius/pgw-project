#ifndef PGW_Server_H
#define PGW_Server_H

#include "pgw/common/types.h"
#include "pgw/server/interfaces/ICdrWriter.h"
#include "pgw/server/interfaces/ISessionManager.h"

#include <memory>
#include <atomic>
#include <string>


// Класс Server управляет жизненным циклом приложения PGW
// Отвечает за Инициализацию компонентов, запуск основного цикла, graceful shutdown

namespace pgw {
namespace server {

class Server {
    // Структура с реализацией
    struct Impl;

    // Указатель на реализацию
    std::unique_ptr<Impl> pImpl;

public:
    explicit Server(const std::string& configPath);
    ~Server();

    // Запрещаем копирование и перемещение (Правило пяти)
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    // Запускает приложение (основной цикл обработки событий)
    int run();

    // Инициирует остановку работы сервера
    void stop();

    // Получение менеджера сессий (для тестов)
    ISessionManager& getSessionManager();

    // Получение CDR писателя (для тестов)
    std::shared_ptr<ICdrWriter> getCdrWriter();
};

} // namespace server
} // namespace pgw

#endif // PGW_Server_H
