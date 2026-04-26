#include "TcpServer.h"
#include "SocketFactory.h"
#include "logger.h"
#include "validation.h"

#include <cstring>
#include <unistd.h> // close()

namespace pgw {

TcpServer::TcpServer(ISessionManager& sessionManager,
                     std::shared_ptr<DatabaseManager> dbManager,
                     pgw::types::constIp_t ip,
                     pgw::types::port_t port)
try : m_sessionManager{sessionManager},
      m_dbManager(std::move(dbManager)),
      m_ip{ip},
      m_port{port},
      m_running{false}{
    LOG_INFO("TCP server initialized");
}
catch(const std::exception& e){
    LOG_ERROR("TCP server initialization  filed: {}", e.what());
    // Пробрасываем исключение дальше
    throw std::runtime_error("TCP server initialization failed");
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
        m_socket = SocketFactory::createTcp(true);  // isListening = true
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

// Метод раcсчитан на использование с менеджером poll, epoll, select
void TcpServer::processEvent(){
    if(!m_running) {
        LOG_INFO("TCP server not running");
        return;
    }

    try{
        // В случае, если на дискрипторе серверного сокета
        // произошло какое-то событие, и poll() нас привел в эту функцию,
        // значит произошло одно из двух:

        // 1. Пришел запрос на новые подключения.
        acceptNewClient();

        // 2. Пришли данные от подключенных клиентов и их надо обработать.
        // Так как мы работаем с сокетом и не хотим его блокировать при работе
        // с данными, мы просто делаем снимок актуальных изменщиков на момент вызова processEvent().
        // Так мы обезопасим себя от итераций по изменяемому контейнеру
        std::vector<int> clientsFd;
        for(const auto& [fd, client] : m_clients){
            clientsFd.push_back(fd);
        }

        for(auto fd : clientsFd){
            handleClientData(fd);
        }
    }
    catch(const std::exception& e){
        LOG_ERROR("TCP server runninig error: {}", e.what());
    }
}

void TcpServer::acceptNewClient(){
    if(!m_running) {
        LOG_INFO("TCP server not running");
        return;
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

            auto clientFd = (*clientSocket)->getFd();
            m_clients[clientFd] = std::move(client);

            // Информация про ip : port выводится при accept() сокетом, а тут подытожим
            LOG_INFO("New TCP client connected: {}", clientFd);
        }
    }
    catch(const std::exception& e){
        LOG_ERROR("Accept client error: {}", e.what());
    }
}


void TcpServer::handleClientData(int clientFd){
    auto& client = m_clients[clientFd];
    if (!client.socket) return;

    try{
        // Неблокирующий режим сокета - наше всё!
        auto packet = client.socket->receive();

        if(packet.data.empty()){
            // TODO: Как-то узнать жив ли еще соккет или нет?
            return;
        }

        client.readBuffer.insert(client.readBuffer.end(), packet.data.begin(), packet.data.end());

        //if(m_commandHandler){
            //Здесь будет парсинг пакета и выполнение команды
            // . . .
            LOG_DEBUG("Received {} bytes from TCP client fd: {}", packet.data.size(), clientFd);
        //}
    }
    catch(const std::exception& e){
        LOG_ERROR("Handle client {} data error: {}",clientFd, e.what());
        removeClient(clientFd);
    }
}

void TcpServer::removeClient(int clientFd){
    auto it = m_clients.find(clientFd);
    if (it != m_clients.end()) {
        if (it->second.socket) {
            it->second.socket->close();
        }
        close(clientFd);
        m_clients.erase(it);
        LOG_DEBUG("TCP client removed, fd: {}", clientFd);
    }
}

} // namespace pgw
