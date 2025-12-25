#include "mocks/logger.h"
#include "mocks/MockCdrWriter.h"
#include "mocks/MockSessionManager.h"
#include "mocks/MockSocket.h"

#include "UdpServer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

struct UdpServerTest : public testing::Test {
    // Будем использовать умные указатели, чтобы лишний раз не инициализировать рессурсы
    std::unique_ptr<MockSessionManager> mockSessionManager;
    std::unique_ptr<MockCdrWriter> mockCdrWriter;
    std::unique_ptr<MockSocket> mockSocket;
    const pgw::types::Ip IP {"127.0.0.1"};
    const pgw::types::Port PORT {8080};
    const sockaddr_in ADDR {AF_INET, htons(12345), htonl(INADDR_ANY)};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        mockSocket = std::make_unique<MockSocket>();
        mockSessionManager = std::make_unique<MockSessionManager>();
        mockCdrWriter = std::make_unique<MockCdrWriter>();
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // . . .
    }
};


TEST_F(UdpServerTest, CreateUdpServer){
    // Ожидаем, что bind будет вызван с правильными параметрами
    EXPECT_CALL(*mockSocket, bind(IP, PORT)).Times(1);

    // Создаем сервер через конструктор с сокетом
    UdpServer server(
        *mockSessionManager,
        *mockCdrWriter,
        std::move(mockSocket),
        IP,
        PORT
    );

    server.start();
    EXPECT_TRUE(server.isRunning());
}