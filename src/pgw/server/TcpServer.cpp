#include "TcpServer.h"

#include "pgw/common/logger.h"
#include "pgw/common/TcpSerializer.h"
#include "pgw/common/SocketFactory.h"

#include <cstring>
#include <unistd.h> // close()

namespace pgw {

TcpServer::TcpServer(pgw::types::constIp_t ip,
                     pgw::types::port_t port,
                     ITcpHandler& commandHandler)
try : m_ip{ip},
      m_port{port},
      m_commandHandler{commandHandler},
      m_socket{SocketFactory::createTcp(true)},
      m_stopAccepting{false},
      m_running{false}{
    LOG_INFO("TCP server initialized");
}
catch(const std::exception& e){
    LOG_ERROR("TCP server initialization  filed: {}", e.what());
    // Пробрасываем исключение дальше
    throw std::runtime_error("TCP server initialization failed");
}

//TODO: возможно слишком много информации в пробрасываемыз исключениях,
// кажется достаточно выписать в LOG_ERROR, а дальше кинуть пустое throw
// В UdpServer то же самое

TcpServer::TcpServer(pgw::types::constIp_t ip,
                     pgw::types::port_t port,
                     ITcpHandler& commandHandler,
                     std::unique_ptr<ITcpSocket> socket)
try : m_ip{ip},
      m_port{port},
      m_commandHandler{commandHandler},
      m_socket{std::move(socket)},
      m_running{false}{
    if (!m_socket) {
        throw std::invalid_argument("TCP socket cannot be nullptr");
    }
    LOG_INFO("TCP test server initialized");
}
catch(const std::exception& e){
    LOG_ERROR("TCP test server initialization filed: {}", e.what());
    // Пробрасываем исключение дальше
    throw std::runtime_error("TCP test server initialization failed");
}


TcpServer::~TcpServer(){
    if(m_running){
        stop();
    }
    LOG_INFO("TCP server deleted");
}

void TcpServer::start(){
    if(m_running) {
        LOG_WARN("TCP server already running");
        return;
    }

    try{
        LOG_INFO("Starting TCP server ...");
        m_socket->bind(m_ip,m_port);
        m_running = true;
    }
    catch (const std::exception& e){
        LOG_ERROR("TCP server start failed {} : {} - {}", m_ip, m_port, e.what());
        // Пробрасываем с дополнительным контекстом
        throw std::runtime_error("TCP server start failed");
    }

    // TODO: Стоит сделать такой же лог в UDP сервере
    LOG_INFO("TCP server started on {} : {}", m_ip, m_port);
}

void TcpServer::stop(){
    if(m_running) {
        LOG_INFO("Stopping TCP server ...");
        m_running = false;

        // Закрываем все клиентские соединения
        for (auto& [fd, client] : m_clients){
            if(client.socket){
                client.socket->close();
            }
            close(fd);
        }
        m_clients.clear();

        // Закрываем серверный сокет
        if(m_socket){
            m_socket->close();
            m_socket.reset();
        }

        LOG_INFO("TCP server stopped");
    }
}

void TcpServer::processEvent(){
    if(!m_running) {
        LOG_INFO("TCP server not running");
        return;
    }

    try{
        // 1. Пришел запрос на новые подключения
        acceptNewClient();

        // 2. Пришли данные от подключенных клиентов и их надо обработать
        for(const auto& [fd, _ ] : m_clients){
            handleClientData(fd);
        }
    }
    catch(const std::exception& e){
        LOG_ERROR("TCP server runninig error: {}", e.what());
    }
}

int TcpServer::acceptNewClient(){
    if(!m_running) {
        LOG_INFO("TCP server not running");
        return -1;
    }
    if(m_stopAccepting){
        LOG_INFO("TCP server stop accepting new clients");
        auto clientSocket = m_socket->accept();
        if (clientSocket.has_value() && clientSocket.value()) {
            int clientFd = clientSocket.value()->getFd();
            LOG_DEBUG("Rejecting new TCP connection: {}", clientFd);
            // Закрываем сразу!
            close(clientFd);
        }
        return -1;
    }

    try{
        // Тут будет nullopt, если неудача.
        // ! Могут выпасть исключения при установке неблокирующего режима !
        auto clientSocket = m_socket->accept();

        // Вот тут nullopt и показывает своё превосходство,
        // потому что для unique_ptr и для nullopt можно использовать метод has_value()
        if(clientSocket.has_value() && clientSocket.value()){

            // Всё, что можем, перемещаем, всё что не можем, тоже перемещаем
            ClientContext client;
            client.socket = std::move(clientSocket.value());
            client.readBuffer.reserve(CLIENT_READ_BUFFER_SIZE);
            client.writeBuffer.reserve(CLIENT_WRITE_BUFFER_SIZE);
            auto clientFd = client.socket->getFd();
            m_clients[clientFd] = std::move(client);

            // Информация про ip : port выводится при accept() сокетом, а тут подытожим
            LOG_INFO("New TCP client connected: {}", clientFd);

            return clientFd;
        }
    }
    catch(const std::exception& e){
        LOG_ERROR("Accept client error: {}", e.what());
        return -1;
    }
    return -1;
}


bool TcpServer::handleClientData(int clientFd){
    // Нет такого дискриптора
    if (m_clients.find(clientFd) == m_clients.end()) return 0;

    // Сокета уже закрылся
    auto& client = m_clients[clientFd];
    if (!client.socket) return 0;

    try{
        // Неблокирующий режим сокета - наше всё!
        auto packet = client.socket->receive();

        if(packet.data.empty()){
            // Проверяем жив ли еще клиент на сокете?
            char buf[1];
            // MSG_PEEK вернет 0, если соединение закрыто
            if (recv(clientFd, buf, 0, MSG_PEEK) == 0) {
                removeClient(clientFd);
            }
            return 0;
        }

        // Сюда читаются все данные с сокета
        // это может быть не одно сообщение, а сразу несколько
        client.readBuffer.insert(client.readBuffer.end(), packet.data.begin(), packet.data.end());

        LOG_DEBUG("Received {} bytes from TCP client fd: {}", packet.data.size(), clientFd);

        // TODO: по хорошему стоит также проверять ввод на неправильные
        // форматы сообщения, потому что в случае одного такого
        // программа будет вечно обходить пакеты стороной из-за одной ошибки.
        // Но пока на этом этапе можно забить на это и верить что клиент всегда
        // будет формировать правильную структуру сообщения (+ KISS ~ my ass)
        // Пытаемся десериализовать сообщения
        while (true) {
            // Считываем первое сообщение из буффера
            auto msg = TcpSerializer::deserializer(client.readBuffer);
            if (!msg.has_value()) {
                break;  // Недостаточно данных для полного сообщения
            }

            // Выполняем команду из сообщения
            auto response = m_commandHandler.handle(*msg);

            // Отправляем ответ на сообщение
            auto responseData = TcpSerializer::serializer(response);
            client.socket->send(std::string(responseData.begin(), responseData.end()));

            // Удаляем обработанное сообщение из буфера
            size_t consumed = sizeof(protocol::MessageHeader) + msg->header.length;
            client.readBuffer.erase(client.readBuffer.begin(), client.readBuffer.begin() + consumed);
        }
        return 1;
    }
    catch(const std::exception& e){
        LOG_ERROR("Handle client {} data error: {}",clientFd, e.what());
        removeClient(clientFd);
        return 0;
    }
}

void TcpServer::removeClient(int clientFd){
    auto it = m_clients.find(clientFd);
    if (it != m_clients.end()) {
        if (it->second.socket) {
            it->second.socket->close();
        }
        m_clients.erase(it);
        LOG_DEBUG("TCP client removed, fd: {}", clientFd);
    }
}

} // namespace pgw
