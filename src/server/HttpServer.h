#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "ISessionManager.h"
#include "ICdrWriter.h"

#include <httplib.h>

#include <memory> // unique_ptr

class HttpServer{
private:
    static inline const std::string HOST {"0.0.0.0"};
    // Ассоциация
    ISessionManager& m_sessionManager;
    // Композиция
    std::unique_ptr<httplib::Server> m_server;
    pgw::types::Port m_port;
    bool m_running;

public:
    // Передаем соккет в конструктор, чтобы тестировать и много не думать
    explicit HttpServer(
        ISessionManager& sessionManager,
        ICdrWriter& m_cdrWriter,
        pgw::types::Port port
    );
    ~HttpServer();

    void start();
    void stop();
    void run();

    bool isRunning() const {return m_running;}

    void setRoutes(); // Сопоставитель запросов к серверу с обработчиками
    void handleCheckSubscriber(const httplib::Request& req, httplib::Response& res);
    void handleStop(const httplib::Request& req, httplib::Response& res);
};


#endif // UDP_SERVER_H