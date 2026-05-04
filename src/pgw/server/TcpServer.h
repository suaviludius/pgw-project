#ifndef PGW_TCP_SERVER_H
#define PGW_TCP_SERVER_H

#include "ITcpSocket.h"
#include "ITcpHandler.h"

#include <memory> // unique_ptr
#include <unordered_map>

// TCP сервер для приема команд управления от HMI клиентов
// Предоставляет интерфейс для интеграции с poll/epoll

namespace pgw {


class TcpServer{
private:
    // Максимальный размер клиентского буффера для чтения (значение наугад)
    static constexpr uint32_t CLIENT_READ_BUFFER_SIZE = 1500;

    // Максимальный размер клиентского буффера для записи (значение наугад)
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
    explicit TcpServer(
        pgw::types::constIp_t ip,
        pgw::types::port_t port,
        ITcpHandler& commandHandler
    );
    ~TcpServer();

    // Запускает UDP сервер (биндит сокет к указанному адресу и порту)
    void start();

    // Останавливает UDP сервер
    void stop();

    // Обработчик событий на серверном сокете.
    // Если данный метод был вызван, значит произошло одно из двух:
    // 1) серверному сокету пришел запрос на установление новой связи с клиентом
    // 2) уже существующий клиент хочет передать данные серверу
    // В общем выполняются два метода acceptNewClient() и handleClientData(fd). Один из них трушный.
    // TODO: Заменить в Udp Server handler() на такое же название: processEvent()
    void processEvent();

    //  Проверяет, запущен ли сервер
    bool isRunning() const {return m_running;}

    // Возвращает файловый дескриптор сокета для использования с poll/epoll
    int getFd() const {return m_socket->getFd();};

    // Проверяет длину, допустимые символы imsi
    bool validateImsi(const std::string& imsi);

private:
    // Подключение новых клиентов
    void acceptNewClient();

    // Обработка данных клиента
    void handleClientData(int fd);

    // Удаление клиента
    void removeClient(int fd);
};

} // namespace pgw

#endif // PGW_TCP_SERVER_H
