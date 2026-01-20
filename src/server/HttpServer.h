#ifndef PGW_HTTP_SERVER_H
#define PGW_HTTP_SERVER_H

// 1. Заголовочный файл модуля
// 2. Собственные заголовки проекта
#include "server/interfaces/ISessionManager.h"
#include "common/types.h"
// 3. Внешние библиотеки
#include <httplib.h>
// 4. Системные С-заголовки
// 5. Стандартные С++ заголовки
#include <memory> // unique_ptr


// HTTP-сервер для управления абонентами PGW
// Предоставляет REST API для проверки статуса абонентов
// и shutdown всей программы

class HttpServer{
private:
    // HTTP коды статусов
    enum class Status {
        // Success
        OK = 200,
        CREATED = 201,
        // Client errors
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        // Server errors
        INTERNAL_SERVER_ERROR = 500,
        SERVICE_UNAVAILABLE = 503
    };

    // Миллисекундная задержка для управления серверными задачами
    static constexpr const int TIMEOUT_MS {100};

    // Хост для привязки сервера
    static constexpr const char* HOST {"0.0.0.0"};

    // Менеджер сессий для проверки статуса абонентов
    ISessionManager& m_sessionManager;

    // HTTP сервер из библиотеки httplib
    std::unique_ptr<httplib::Server> m_server;

    // Порт, на котором слушает сервер
    pgw::types::Port m_port;

    // Флаг состояния сервера (true = запущен, false = остановлен)
    bool m_running;

    // Поток работы http сервера
    std::thread m_serverThread;

    // Ссылка на атомик завершения сессии (передает сигнал наружу)
    std::atomic<bool>& m_shutdownRequest;

public:
    explicit HttpServer(
        ISessionManager& sessionManager,
        pgw::types::Port port,
        std::atomic<bool>& shutdownRequest
    );
    ~HttpServer();

    // Запрещаем копирование и присваивание (Правило пяти).
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&) = delete;
    HttpServer& operator=(HttpServer&&) = delete;

    // Запускает HTTP сервер в отдельном потоке
    void start();

    // Завершает текущие запросы и освобождает ресурсы
    void stop();

    // Проверяет, запущен ли сервер
    bool isRunning() const {return m_running;}

    // Настраивает маршруты HTTP сервера
    void setRoutes();

    // Обрабатывает запрос проверки статуса абонента
    void handleSubscriberCheck(const httplib::Request& req, httplib::Response& res);

    // Обрабатывает запрос graceful shutdown сервера
    void handleShutdown(const httplib::Request& req, httplib::Response& res);
};

#endif // PGW_UDP_SERVER_H