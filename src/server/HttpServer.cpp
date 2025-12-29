#include "HttpServer.h"
#include "logger.h"

HttpServer::HttpServer(ISessionManager& sessionManager,
                     ICdrWriter& cdrWriter,
                     pgw::types::Port port)
    : m_sessionManager{sessionManager},
      m_port{port},
      m_server{std::make_unique<httplib::Server>()},
      m_running{false}{
    setRoutes(); // Настраиваем серверные запросы до bind() сервера
    Logger::info("UDP server initialized");
}

HttpServer::~HttpServer(){
    stop();
}

void HttpServer::start(){
    if(m_running) {
        Logger::warn("HTTP server already running");
        return;
    }
    try{
        m_server->bind_to_port(HOST, m_port);
        // Проверяем, что сервер запустился
        if (!m_server->is_valid()) {
            throw std::runtime_error("HTTP server invalid binding to port " + std::to_string(m_port));
        }
        m_running = true;
    }
    catch (const std::exception& e){
        Logger::error("HTTP server start failed: " + std::to_string(m_port) + " - " + std::string(e.what()));
        // Пробрасываем с дополнительным контекстом
        std::throw_with_nested(std::runtime_error("HTTP server start failed"));
    }
    Logger::info("HTTP server sucseccdully start");
    // run();
}

void HttpServer::stop(){
    if(!m_running) return;
    m_server->stop();
    m_running = false;
    Logger::info("HTTP server stopped");
}

void HttpServer::run(){
    if(!m_running) return;
    try{
        m_server->listen_after_bind();
    }
    catch(const std::exception& e){
        Logger::error("UDP server runninig error: " + std::string(e.what()));
    }
}

void HttpServer::setRoutes() {
    // Проверка абонентов
    // Клиент → GET http://сервер:порт/check_subscriber?imsi=123... → Сервер → метод handleCheckSubscriber()
    m_server->Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res){
        handleCheckSubscriber(req,res);
    });
    // Остановка (graceful) сервера
    // Клиент → POST http://сервер:порт/stop (+ может еще что-то?) → Сервер → метод handleStop()
    m_server->Post("/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleStop(req, res);
    });

    // Если запрос не соответсвтует описанным выше
    m_server->set_error_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_content("Not Found", "text/plain");
        res.status = 404; // Not found
    });
}

void HttpServer::handleCheckSubscriber(const httplib::Request& req, httplib::Response& res){
    auto imsi = req.get_param_value("imsi");
    if(imsi.empty()){
        res.set_content("Request error: no imsi", "text/plan");
        res.status = 400; // Bad request
        Logger::warn("HTTP server check subscriber request: missing imsi parameter");
        return;
    }
    Logger::info("HTTP server request: check subscriber with imsi: " + imsi);
    std::string status = m_sessionManager.hasSession(imsi) ? "active" : "not active";
    res.set_content(status, "text/plan");
    res.status = 200; // OK
    Logger::info("HTTP server response: subscriber status is " + status);
}

void HttpServer::handleStop(const httplib::Request& req, httplib::Response& res){
    Logger::info("HTTP server request: graceful shutdown");
    // m_sessionManager.gracefulShutdown(); // -- Нужно задать через атомик флаг или шаред мемори в главный поток программы
    res.set_content("Graceful shutdown flag set", "text/plain");
    // Можно в цикле ожидать выставление флага внутри sessionManager,
    // который скажет о сбросе, определенное время, если он не появится
    // то выдать ошибку выполнения в реквесте.
    res.status = 200; // OK
    Logger::info("HTTP server response: graceful shutdown flag set");
}