#ifndef PGW_TCP_SERVER_H
#define PGW_TCP_SERVER_H

#include "pgw/common/interfaces/ITcpSocket.h"
#include "ITcpHandler.h"

#include <memory> // unique_ptr
#include <unordered_map>

// TCP сервер для приема команд управления от HMI клиентов
// Предоставляет интерфейс для интеграции с poll/epoll

namespace pgw {


class TcpServer{
private:
    // Максимальный размер клиентского буффера для чтения
    static constexpr uint32_t CLIENT_READ_BUFFER_SIZE = 1500;

    // Максимальный размер клиентского буффера для записи
    static constexpr uint32_t CLIENT_WRITE_BUFFER_SIZE = 1500;

    // Обработчик команд (создается извне)
    ITcpHandler& m_commandHandler;

    // Умный указатель на слушающий сокет (создаем внутри)
    std::unique_ptr<ITcpSocket> m_socket;

    // IP-адрес сервера
    pgw::types::ip_t m_ip;

    // Порт сервера
    pgw::types::port_t m_port;

    bool m_running;

    // Каждый клиент может читать и писать на своём сокете
    struct ClientContext{
        std::unique_ptr<ITcpSocket> socket;
        std::vector<uint8_t> writeBuffer;
        std::vector<uint8_t> readBuffer;
        bool isWriting = false;
    };

    // Буффер из клиентских соединение [fd, client]
    std::unordered_map<int, ClientContext> m_clients;

public:
    // Продакшн коснтруктор
    TcpServer(
        pgw::types::constIp_t ip,
        pgw::types::port_t port,
        ITcpHandler& commandHandler
    );
    // Тестовый конструктор
    TcpServer(
        pgw::types::constIp_t ip,
        pgw::types::port_t port,
        ITcpHandler& commandHandler,
        std::unique_ptr<ITcpSocket> socket
    );
    ~TcpServer();

    // Запускает UDP сервер (биндит сокет к указанному адресу и порту)
    void start();

    // Останавливает UDP сервер
    void stop();

    // Выполняет два метода: acceptNewClient() и handleClientData(fd)
    void processEvent();

    // Подключение новых клиентов, возвращает фаловый дискриптор
    int acceptNewClient();

    // Обработка данных клиента, возвращает статус соединения 1 - активно, 0 - закрыто
    bool handleClientData(int fd);

    // Проверяет, запущен ли сервер
    bool isRunning() const {return m_running;}

    // Возвращает файловый дескриптор сокета сервера для использования с poll/epoll
    int getFd() const {return m_socket->getFd();}

    size_t getClientsCount() { return m_clients.size();}

private:
    // Удаление клиента
    void removeClient(int fd);
};

} // namespace pgw

#endif // PGW_TCP_SERVER_H
