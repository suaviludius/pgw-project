#ifndef PGW_APPLICATION_H
#define PGW_APPLICATION_H

#include "types.h"
#include "ISessionManager.h"
#include "ICdrWriter.h"

#include <memory>
#include <atomic>
#include <string>

// Forward declarations
namespace pgw {

class DatabaseManager;
class SessionManager;
class TcpServer;
class UdpServer;
class HttpServer;

// Класс Application управляет жизненным циклом приложения PGW
// Отвечает за Инициализацию компонентов, запуск основного цикла, graceful shutdown
namespace server {

class Config;

class Application {
public:
    explicit Application(const std::string& configPath);
    ~Application();

    // Запрещаем копирование и перемещение (Правило пяти)
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    // Запускает приложение (основной цикл обработки событий)
    int run();

private:
    // Инициализация компонентов
    bool initializeLogger();
    bool initializeDatabase();
    bool initializeCdrWriter();
    bool initializeSessionManager();
    bool initializeServers();

    // Основной цикл обработки событий
    void runEventLoop();

    // Graceful shutdown
    void shutdown();

    // Конфигурация приложения (создается один раз в конструкторе)
    std::unique_ptr<Config> m_config;

    // Флаг завершения работы
    std::atomic<bool> m_shutdownRequest{false};

    // Компоненты приложения
    std::shared_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ICdrWriter> m_cdrWriter;
    std::unique_ptr<SessionManager> m_sessionManager;
    std::unique_ptr<TcpServer> m_tcpServer;
    std::unique_ptr<UdpServer> m_udpServer;
    std::unique_ptr<HttpServer> m_httpServer;
};

} // namespace server
} // namespace pgw

#endif // PGW_APPLICATION_H
