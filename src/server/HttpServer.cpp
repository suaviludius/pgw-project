#include "HttpServer.h"
#include "logger.h"

HttpServer::HttpServer(
    ISessionManager& sessionManager,
    pgw::types::Port port,
    std::atomic<bool>& shutdownRequest
) : m_sessionManager{sessionManager},
    m_port{port},
    m_shutdownRequest{shutdownRequest},
    m_server{std::make_unique<httplib::Server>()},
    m_running{false}{
    setRoutes(); // Настраиваем серверные запросы до bind() сервера
    LOG_INFO("HTTP server initialized");
}

HttpServer::~HttpServer(){
    stop();
}

void HttpServer::start(){
    if(m_running) {
        LOG_WARN("HTTP server already running");
        return;
    }
    try{
        m_running = true;
        // Запускаем сервер в отдельном потоке
        m_serverThread = std::thread([this]() {
            try {
                LOG_INFO("HTTP server listening on port {}", m_port);
                // Эта функция БЛОКИРУЕТ поток
                m_server->listen(HOST, m_port);
            } catch (const std::exception& e) { // Все исключения должны обрабатываться внутри потока, иначе terminate
                if (m_running) {  // Логируем только если не остановлен
                    LOG_ERROR("HTTP server error: {}", e.what());
                }
            }
        });
    }
    catch (const std::exception& e){
        m_running = false;
        LOG_ERROR("HTTP server start failed: {} - {}", m_port, e.what());
        // Пробрасываем с дополнительным контекстом
        std::throw_with_nested(std::runtime_error("HTTP server start failed"));
    }
    LOG_INFO("HTTP server sucseccdully start");
}

void HttpServer::stop(){
    if(!m_running) {
        LOG_INFO("HTTP server already stopped");
        return;
    }
    LOG_INFO("Stopping HTTP server...");
    m_running = false;

    // Даем время для обработки текущих запросов
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Останавливаем сервер
    m_server->stop();

    // Ждем завершения потока
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }

    LOG_INFO("HTTP server stopped");
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
    // Обработчик для проверки флага остановки
    m_server->set_pre_routing_handler([this](const httplib::Request&, httplib::Response& res) {
        if(!m_running){
            res.status = 503; // Service Unavaliable
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });
}

void HttpServer::handleCheckSubscriber(const httplib::Request& req, httplib::Response& res){
    auto imsi = req.get_param_value("imsi");
    if(imsi.empty()){
        res.set_content("Request error: no imsi", "text/plan");
        res.status = 400; // Bad request
        LOG_WARN("HTTP server check subscriber request: missing imsi parameter");
        return;
    }
    LOG_INFO("HTTP server request: check subscriber with imsi: " + imsi);
    std::string status = m_sessionManager.hasSession(imsi) ? "active" : "not active";
    res.set_content(status, "text/plan");
    res.status = 200; // OK
    LOG_INFO("HTTP server response: subscriber status is " + status);
}

void HttpServer::handleStop(const httplib::Request& req, httplib::Response& res){
    LOG_INFO("HTTP server request: graceful shutdown");
    // Передаем запрос менеджеру сессий на удаление сессий
    m_shutdownRequest.store(true, std::memory_order_release);
    res.set_content("Graceful shutdown request set", "text/plain");
    // Можно в цикле ожидать выставление флага внутри sessionManager,
    // который скажет о сбросе, определенное время, если он не появится
    // то выдать ошибку выполнения в реквесте.
    res.status = 200; // OK
    LOG_INFO("HTTP server response: graceful shutdown request set");
}