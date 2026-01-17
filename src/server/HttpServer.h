#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "ISessionManager.h"
#include "ICdrWriter.h"

#include <httplib.h>

#include <memory> // unique_ptr

class HttpServer{
private:
    static constexpr const char* HOST {"0.0.0.0"};
    // Ассоциация
    ISessionManager& m_sessionManager;
    // Композиция
    std::unique_ptr<httplib::Server> m_server;
    pgw::types::Port m_port;
    bool m_running;

    // Поток работы http сервера
    std::thread m_serverThread;
    // Ссылка на атомик из приложения завершения сессий
    std::atomic<bool>& m_shutdownRequest;

public:
    // Передаем соккет в конструктор, чтобы тестировать и много не думать
    explicit HttpServer(
        ISessionManager& sessionManager,
        pgw::types::Port port,
        std::atomic<bool>& shutdownRequest
    );
    ~HttpServer();

    void start(); // Создает отдельный поток для работы сервера
    void stop();

    bool isRunning() const {return m_running;}

    void setRoutes(); // Сопоставитель запросов к серверу с обработчиками
    void handleCheckSubscriber(const httplib::Request& req, httplib::Response& res);
    void handleStop(const httplib::Request& req, httplib::Response& res);
};


#endif // UDP_SERVER_H