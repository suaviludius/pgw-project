#include "logger.h"
#include "TcpServer.h"
#include "TcpSerializer.h"
#include "MockTcpHandler.h"
#include "MockTcpSocket.h"

#include <gtest/gtest.h>

// Фикстура для всех тестов
struct TcpServerTest : public testing::Test {

    // Набор заглушек
    std::unique_ptr<MockTcpHandler> mockHandler;
    std::unique_ptr<MockTcpSocket> mockServerSocket;
    // Так как mockSocket будет перемещен в tcpServer,
    // а мне нужно как-то отлавить EXPECT_CALL для сокета
    // внутри tcpServer, то сохраняем его адрес на память
    MockTcpSocket* rawServerSocket;
    // Набор тестовых констант
    const pgw::types::ip_t IP {"0.0.0.0"};
    const pgw::types::port_t PORT {9090};
    const pgw::types::imsi_t IMSI1 {"123456789012345"};
    const pgw::types::imsi_t IMSI2 {"123456789012346"};
    const int CLIENT_FD = 7;
    const size_t CLIENTS_COUNT = 10;
    // Сам TcpServer
    std::unique_ptr<pgw::TcpServer> tcpServer;

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        mockHandler = std::make_unique<MockTcpHandler>();
        mockServerSocket = std::make_unique<MockTcpSocket>();
        rawServerSocket = mockServerSocket.get();
        tcpServer = std::make_unique<pgw::TcpServer>(IP,PORT,*mockHandler,std::move(mockServerSocket));
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        if (tcpServer && tcpServer->isRunning()) {
            tcpServer->stop();
        }
    }
};


TEST_F(TcpServerTest, StartStopServer_Success){
    // Arrange
    EXPECT_CALL(*rawServerSocket, bind(IP, PORT))
        .Times(1);
    EXPECT_CALL(*rawServerSocket, close())
        .Times(1);

    EXPECT_NO_THROW({
        // Act
        tcpServer->start();
        // Assert
        EXPECT_TRUE(tcpServer->isRunning());
        // Act
        tcpServer->stop();
        // Assert
        EXPECT_FALSE(tcpServer->isRunning());
    });
}

TEST_F(TcpServerTest, AcceptNewClient_Success) {
    // Arrange
    auto mockClientSocket = std::make_unique<MockTcpSocket>();
    auto rawClientSocket = mockClientSocket.get();

    // start
    EXPECT_CALL(*rawServerSocket, bind(IP, PORT))
        .Times(1);

    // processEvent
    // 1 - acceptNewClient
    EXPECT_CALL(*rawServerSocket, accept())
        .Times(1)
        .WillOnce(testing::Return(std::optional(std::move(mockClientSocket))));

    EXPECT_CALL(*rawClientSocket, getFd())
        .Times(1)
        .WillRepeatedly(testing::Return(CLIENT_FD));

    // 2 - handleClientData
    EXPECT_CALL(*rawClientSocket, receive())
        .Times(1)
        .WillOnce(testing::Return(pgw::ITcpSocket::Packet{}));

    // stop
    EXPECT_CALL(*rawServerSocket, close())
        .Times(1);
    EXPECT_CALL(*rawClientSocket, close())
        .Times(1);

    // Act
    tcpServer->start();
    tcpServer->processEvent();

    // Assert
    EXPECT_EQ(tcpServer->getClientsCount(), 1);

    // Act
    tcpServer->stop();
}

TEST_F(TcpServerTest, HandleClientData_Success) {
    // Arrange
    auto mockClientSocket = std::make_unique<MockTcpSocket>();
    auto rawClientSocket = mockClientSocket.get();

    // Request
    auto requestMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::TEST,
        pgw::protocol::Status::TEST,
        {{"test", "request"}}
    );
    auto binaryRequest = pgw::TcpSerializer::serializer(requestMsg);
    std::string stringRequest (binaryRequest.begin(),binaryRequest.end());

    // Response
    auto responseMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::TEST,
        pgw::protocol::Status::TEST,
        {{"test", "response"}}
    );

    // start
    EXPECT_CALL(*rawServerSocket, bind(IP, PORT))
        .Times(1);

    // processEvent
    // 1 - acceptNewClient
    EXPECT_CALL(*rawServerSocket, accept())
        .Times(1)
        .WillOnce(testing::Return(std::optional(std::move(mockClientSocket))));

    EXPECT_CALL(*rawClientSocket, getFd())
        .Times(1)
        .WillOnce(testing::Return(CLIENT_FD));

    // 2 - handleClientData
    EXPECT_CALL(*rawClientSocket, receive())
        .Times(1)
        .WillOnce(testing::Return(pgw::ITcpSocket::Packet{ .data = stringRequest }));

    EXPECT_CALL(*mockHandler, handle(testing::_))
        .Times(1)
        .WillOnce(testing::Return(responseMsg));

    EXPECT_CALL(*rawClientSocket, send(testing::_))
        .Times(1);

    // stop
    EXPECT_CALL(*rawServerSocket, close())
        .Times(1);
    EXPECT_CALL(*rawClientSocket, close())
        .Times(1);


    // Act
    tcpServer->start();
    tcpServer->processEvent();

    // Assert
    EXPECT_EQ(tcpServer->getClientsCount(), 1);

    // Act
    tcpServer->stop();

}

TEST_F(TcpServerTest, ManyClientsHandle_Succsess) {
    // Arrange
    std::vector<std::unique_ptr<MockTcpSocket>> mockClientSockets;
    std::vector<MockTcpSocket*> rawClientSockets;
    std::vector<int> clientFds;

    mockClientSockets.reserve(CLIENTS_COUNT);
    rawClientSockets.reserve(CLIENTS_COUNT);
    clientFds.reserve(CLIENTS_COUNT);

    for (size_t i = 0; i < CLIENTS_COUNT; ++i) {
        // Умные указатели можно размещать сразу внутри, избегая лишних копирований
        mockClientSockets.emplace_back(std::make_unique<MockTcpSocket>());
        rawClientSockets.push_back(mockClientSockets[i].get());
        clientFds.push_back(CLIENT_FD+static_cast<int>(i));
    }

    // Request
    auto requestMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::TEST,
        pgw::protocol::Status::TEST,
        {{"test", "request"}}
    );
    auto binaryRequest = pgw::TcpSerializer::serializer(requestMsg);
    std::string stringRequest(binaryRequest.begin(), binaryRequest.end());

    // Response
    auto responseMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::TEST,
        pgw::protocol::Status::TEST,
        {{"test", "response"}}
    );

    // start
    EXPECT_CALL(*rawServerSocket, bind(IP, PORT))
        .Times(1);


    // server

    // Когда все клиенты будут добавлены, то на processEvent
    // будет снова выполняться acceptNewClient, но возвращать nullopt,
    // так как новых клиентов не будет, а будут просто приходить данные
    // от уже созданных клиентов

    // Вот эта шляпа была вынужденной мерой, потому что цикл с использованием std::move()
    // поверх EXEPT_CALL совершал насилие над моими нервными клетками
    int nextClientIndex = 0;
    EXPECT_CALL(*rawServerSocket, accept())
        .WillRepeatedly(testing::Invoke([&mockClientSockets, &nextClientIndex]() mutable {
            return std::move(mockClientSockets[nextClientIndex++]);
        }));

    EXPECT_CALL(*rawServerSocket, close())
        .Times(1);

    // client
    for (size_t i = 0; i < CLIENTS_COUNT; ++i) {
        // processEvent
        // 1 - acceptNewClient
        EXPECT_CALL(*rawClientSockets[i], getFd())
            .Times(1)
            .WillOnce(testing::Return(clientFds[i]));

        // 2 - handleClientData
        // Так как при наличи данных на tcpServer сокете проверяются все клиентские файловые дискрипторы,
        // то при одном чтении сокета для каждого подключенного клиента будет вызываться мок receive
        // с заданными параметрами, поэтому стоит сделать так, чтобы на моке отправка данных была только при первом
        // прочтении сокета каждого из клиентов, а при повторном чтении на том же сокете было пусто (типо больше не присылали)
        EXPECT_CALL(*rawClientSockets[i], receive())
            .WillOnce(testing::Return(pgw::ITcpSocket::Packet{.data = stringRequest}))
            .WillRepeatedly(testing::Return(pgw::ITcpSocket::Packet{}));  // Пустые пакеты после первого

        EXPECT_CALL(*rawClientSockets[i], send(testing::_))
            .Times(testing::AtLeast(1))
            .WillRepeatedly(testing::Return());

        EXPECT_CALL(*rawClientSockets[i], close())
            .Times(1);
    }

    EXPECT_CALL(*mockHandler, handle(testing::_))
        // Хотя бы по одному разу на клиента
        .Times(testing::AtLeast(CLIENTS_COUNT))
        .WillRepeatedly(testing::Return(responseMsg));


    // Act
    tcpServer->start();

    for (int i = 0; i < CLIENTS_COUNT; ++i) {
        tcpServer->processEvent();
    }

    // Assert
    EXPECT_EQ(tcpServer->getClientsCount(), CLIENTS_COUNT);

    // Act
    tcpServer->stop();
}
