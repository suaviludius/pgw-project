#include "server/HttpServer.h"

#include "common/logger.h"

HttpServer::HttpServer(
    ISessionManager& sessionManager,
    pgw::types::Port port,
    std::atomic<bool>& shutdownRequest)
    : m_sessionManager{sessionManager},
      m_port{port},
      m_shutdownRequest{shutdownRequest},
      m_running{false}{
    try {
        // Создаем указатель на библиотечный сервер
        m_server = std::make_unique<httplib::Server>();

        // Настраиваем маршруты до запуска сервера
        setRoutes();
    }
    catch(const std::exception& e){
        LOG_ERROR("HTTP server init failed: {}", e.what());
    }

    LOG_INFO("HTTP server initialized");
}

HttpServer::~HttpServer(){
    // Если сервер не был остановлен
    if(m_running) {
        // Деструктор не должен выбрасывать исключения
        try {
            stop();
        }
        catch (const std::exception& e){
            LOG_ERROR("HTTP server destroy failed: {} - {}", m_port, e.what());
        }
    }
}

void HttpServer::start(){
    if(m_running) {
        LOG_WARN("HTTP server already running");
        return;
    }

    try{
        m_running = true;

        // Запускаем HTTP сервер в отдельном потоке, т.к. httplib::listen() блокирующий
        m_serverThread = std::thread([this]() {
            try {
                LOG_INFO("HTTP server listening on port {}", m_port);
                m_server->listen(HOST, m_port);
            } catch (const std::exception& e) { // Все исключения должны обрабатываться внутри потока, иначе terminate
                if (m_running) {  // Если сервер остановился, то не логируем исключения
                    LOG_ERROR("HTTP server error: {}", e.what());
                }
            }
        });

        // Даем время потоку запуститься
        std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
    }
    catch (const std::exception& e){
        m_running = false;
        LOG_ERROR("HTTP server start failed: {} - {}", m_port, e.what());
    }

    LOG_INFO("HTTP server sucseccdully start");
}

void HttpServer::stop(){
    if(!m_running) {
        LOG_INFO("HTTP server already stopped");
        return;
    }

    try{
        LOG_INFO("Stopping HTTP server...");

        m_running = false;

        // Даем время для обработки текущих запросов перед остановкой сервера
        std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
        m_server->stop();

        // Ждем завершения потока
        if (m_serverThread.joinable()) {
            m_serverThread.join();
        }
    }
    catch (const std::exception& e){
        LOG_ERROR("HTTP server stop failed: {} - {}", m_port, e.what());
        m_running = false;
    }

    LOG_INFO("HTTP server stopped");
}

void HttpServer::setRoutes() {
    // До начала маршрутизации трафика проверяем не остановлен ли сервер
    m_server->set_pre_routing_handler([this](const httplib::Request&, httplib::Response& res) {
        if(!m_running){
            res.status = static_cast<int>(Status::SERVICE_UNAVAILABLE);

            // Говорим httplib, что запрос обработан, пусть переходит к следующим запросам
            return httplib::Server::HandlerResponse::Handled;
        }

        // Говорим httplib, что запрос не обработан, нужно его отрутинить
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Проверка абонентов
    // Клиент → GET http://сервер:порт/check_subscriber?imsi=123... → Сервер → метод handleCheckSubscriber()
    m_server->Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res){
        handleSubscriberCheck(req,res);
    });

    // Остановка (shutdown) сервера
    // Клиент → POST http://сервер:порт/stop (+ может еще что-то?) → Сервер → метод handleStop()
    m_server->Post("/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleShutdown(req, res);
    });

    // Если запрос не соответсвтует задачам выше
    m_server->set_error_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_content("Not Found", "text/plain");
        res.status = static_cast<int>(Status::NOT_FOUND); // Not found
    });
}

void HttpServer::handleSubscriberCheck(const httplib::Request& req, httplib::Response& res){
    // Извлекаем imsi из запроса
    auto imsi = req.get_param_value("imsi");
    if(imsi.empty()){
        // Параметр отсутствует — возвращаем ошибку клиенту
        res.set_content("Request error: no imsi", "text/plain");
        res.status = static_cast<int>(Status::BAD_REQUEST);
        LOG_WARN("HTTP server check subscriber request: missing imsi parameter");
        return;
    }

    LOG_INFO("HTTP server request: check subscriber with imsi: " + imsi);

    // Проверка наличия активной сессии через SessionManager
    std::string status = m_sessionManager.hasSession(imsi) ? "active" : "not active";

    // Формирование успешного ответа
    res.set_content(status, "text/plain");
    res.status = static_cast<int>(Status::OK);
    LOG_INFO("HTTP server response: subscriber status is " + status);
}

void HttpServer::handleShutdown(const httplib::Request& req, httplib::Response& res){
    LOG_INFO("HTTP server request: graceful shutdown");

    // Атомарно устанавливаем флаг запроса на завершение работы
    m_shutdownRequest.store(true, std::memory_order_release);

    // Отправляем клиенту подтверждение приёма запроса
    res.set_content("Graceful shutdown request set", "text/plain");
    res.status = static_cast<int>(Status::OK);
    LOG_INFO("HTTP server response: graceful shutdown request set");
}