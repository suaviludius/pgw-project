#ifndef PGW_UDP_SERVER_H
#define PGW_UDP_SERVER_H

#include "ISessionManager.h"
#include "ICdrWriter.h"
#include "ISocket.h"

#include <memory> // unique_ptr

// UDP сервер для обработки пакетов от абонентов
// Принимает UDP датаграммы, валидирует IMSI и передает управление SessionManager
// Предоставляет интерфейс для интеграции с poll/epoll

class UdpServer{
private:
    // Максимальный размер принимаемого UDP пакета
    static constexpr uint32_t MAX_DATAGRAM_SIZE = 1500;

    // Максимальная длина IMSI
    static constexpr size_t MAX_IMSI_LENGTH = 15;

    // Ссылка на менеджер сессий (ассоциация)
    ISessionManager& m_sessionManager;

    // Умный указатель на сокет
    std::unique_ptr<ISocket> m_socket;

    // IP-адрес сервера
    pgw::types::Ip m_ip;

    // Порт сервера
    pgw::types::Port m_port;

    bool m_running;

public:
    explicit UdpServer(
        ISessionManager& sessionManager,
        pgw::types::ConstIp ip,
        pgw::types::Port port,
        std::unique_ptr<ISocket> socket = nullptr
    );
    ~UdpServer();

    // Запускает UDP сервер (биндит сокет к указанному адресу и порту)
    void start();

    // Останавливает UDP сервер
    void stop();

    // Обработчик входящих пакетов. Вызывается из основного цикла при наличии данных
    // Читает доступные датаграммы, валидирует IMSI и создает сессии
    void handler();

    //  Проверяет, запущен ли сервер
    bool isRunning() const {return m_running;}

    // Возвращает файловый дескриптор сокета для использования с poll/epoll
    int getFd() const {return m_socket->getFd();};

    // Проверяет длину, допустимые символы imsi
    bool validateImsi(const std::string& imsi);
};

#endif // PGW_UDP_SERVER_H