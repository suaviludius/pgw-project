#include "logger.h"
#include "MockSessionManager.h"
#include "MockSocket.h"

#include "UdpServer.h"
#include "Socket.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

struct UdpServerTest : public testing::Test {
    // Будем использовать умные указатели, чтобы лишний раз не инициализировать рессурсы
    std::unique_ptr<MockSessionManager> mockSessionManager;
    std::unique_ptr<MockSocket> mockSocket;
    const pgw::types::ip_t IP {"0.0.0.0"};
    const pgw::types::port_t PORT {1};
    const pgw::types::imsi_t IMSI1 {"012340123401234"};
    const pgw::types::imsi_t IMSI2 {"000111222333444"};
    const sockaddr_in ADDR {AF_INET, htons(8080), htonl(INADDR_ANY)};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        mockSocket = std::make_unique<MockSocket>();
        mockSessionManager = std::make_unique<MockSessionManager>();
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // . . .
    }
};


TEST_F(UdpServerTest, StartUdpServer){
    // Arrange
    // Ожидаем, что bind будет вызван с правильными параметрами и ровно 1 раз
    EXPECT_CALL(*mockSocket, bind(IP, PORT))
        .Times(1);

    // Создаем сервер через конструктор с сокетом
    EXPECT_NO_THROW({
        UdpServer server(*mockSessionManager,IP,PORT, std::move(mockSocket));
        // Act
        server.start();
        // Assert
        EXPECT_TRUE(server.isRunning());
        // Act
        server.stop();
        // Assert
        EXPECT_FALSE(server.isRunning());
    });
}

TEST_F(UdpServerTest, ValidateImsi) {
    UdpServer server(*mockSessionManager, IP, PORT, std::move(mockSocket));

    // Корректные IMSI
    EXPECT_TRUE(server.validateImsi(IMSI1));
    EXPECT_TRUE(server.validateImsi(IMSI2));

    // Некорректные IMSI
    EXPECT_FALSE(server.validateImsi(""));                    // Пустая строка
    EXPECT_FALSE(server.validateImsi("123"));                 // Слишком короткий
    EXPECT_FALSE(server.validateImsi("1234567890123456"));    // Слишком длинный
    EXPECT_FALSE(server.validateImsi("1234567890abcde"));     // Буквы вместо цифр
    EXPECT_FALSE(server.validateImsi("12345-789012345"));     // Символ дефиса
    EXPECT_FALSE(server.validateImsi("12345678901234 "));     // Пробел в конце
    EXPECT_FALSE(server.validateImsi(" 123456789012345"));    // Пробел в начале
}

// Перегружаем оператор сравнения чтобы GTest мог сравнивать структуры сокетов
bool operator==(const sockaddr_in& lhs, const sockaddr_in& rhs) {
    return lhs.sin_family == rhs.sin_family &&
           lhs.sin_port == rhs.sin_port &&
           lhs.sin_addr.s_addr == rhs.sin_addr.s_addr;
}

TEST_F(UdpServerTest, RunWithValidImsi){
    // Arrange
    ISocket::Packet testPacket {IMSI1, ADDR};

    EXPECT_CALL(*mockSocket, bind(IP, PORT))
        .Times(1);

    EXPECT_CALL(*mockSocket, receive())
        .Times(2)
        .WillOnce(testing::Return(testPacket))            // При вызове receive() вернём testPacket
        .WillOnce(testing::Return(ISocket::Packet{}));    // Второй пакет будет пустой

    EXPECT_CALL(*mockSessionManager, createSession(IMSI1))
        .Times(1)
        .WillOnce(testing::Return(ISessionManager::CreateResult::CREATED));

    EXPECT_CALL(*mockSocket, send("created", ADDR))
        .Times(1); // Ожидаем вызов send() с параметрами created и ADDR

    // Создаем сервер
    UdpServer server(*mockSessionManager,IP,PORT, std::move(mockSocket));
    // Act
    server.start();
    server.handler();
    // Assert
    EXPECT_TRUE(server.isRunning());
}

TEST_F(UdpServerTest, RunWithInvalidImsi) {
    // Arrange
    ISocket::Packet testPacket {"8-800-555-35-35", ADDR};

    EXPECT_CALL(*mockSocket, bind(IP, PORT))
        .Times(1);

    EXPECT_CALL(*mockSocket, receive())
        .Times(2)
        .WillOnce(testing::Return(testPacket))            // При вызове receive() вернём testPacket
        .WillOnce(testing::Return(ISocket::Packet{}));    // Второй пакет будет пустой

    EXPECT_CALL(*mockSessionManager, createSession(IMSI1))
        .Times(0);

    EXPECT_CALL(*mockSocket, send("rejected", ADDR))
        .Times(1); // Ожидаем вызов send() с параметрами created и ADDR

    // Создаем сервер
    UdpServer server(*mockSessionManager,IP,PORT, std::move(mockSocket));
    // Act
    server.start();
    server.handler();
    // Assert
    EXPECT_TRUE(server.isRunning());
}

TEST_F(UdpServerTest, RunWithBlacklistedImsi) {
    // Arrange
    ISocket::Packet testPacket {IMSI1, ADDR};

    EXPECT_CALL(*mockSocket, bind(IP, PORT))
        .Times(1);

    EXPECT_CALL(*mockSocket, receive())
        .Times(2)
        .WillOnce(testing::Return(testPacket))            // При вызове receive() вернём testPacket
        .WillOnce(testing::Return(ISocket::Packet{}));    // Второй пакет будет пустой

    EXPECT_CALL(*mockSessionManager, createSession(IMSI1))
        .Times(1)
        .WillOnce(testing::Return(ISessionManager::CreateResult::REJECTED_BLACKLIST));

    EXPECT_CALL(*mockSocket, send("rejected", ADDR))
        .Times(1); // Ожидаем вызов send() с параметрами created и ADDR

    // Создаем сервер
    UdpServer server(*mockSessionManager, IP,PORT, std::move(mockSocket));
    // Act
    server.start();
    server.handler();
    // Assert
    EXPECT_TRUE(server.isRunning());
}

TEST_F(UdpServerTest, RunWithAlreadyExistedImsi) {
    // Arrange
    ISocket::Packet testPacket {IMSI1, ADDR};

    EXPECT_CALL(*mockSocket, bind(IP, PORT))
        .Times(1);

    EXPECT_CALL(*mockSocket, receive())
        .Times(2)
        .WillOnce(testing::Return(testPacket))            // При вызове receive() вернём testPacket
        .WillOnce(testing::Return(ISocket::Packet{}));    // Второй пакет будет пустой

    EXPECT_CALL(*mockSessionManager, createSession(IMSI1))
        .Times(1)
        .WillOnce(testing::Return(ISessionManager::CreateResult::ALREADY_EXISTS));


    EXPECT_CALL(*mockSocket, send("rejected", ADDR))
        .Times(1); // Ожидаем вызов send() с параметрами created и ADDR


    // Создаем сервер
    UdpServer server(*mockSessionManager, IP,PORT, std::move(mockSocket));
    // Act
    server.start();
    server.handler();
    // Assert
    EXPECT_TRUE(server.isRunning());
}

TEST_F(UdpServerTest, RunWithoutThrows) {
    // Arrange
    EXPECT_CALL(*mockSocket, bind("127.0.0.1", 8080))
        .Times(1);

    EXPECT_CALL(*mockSocket, receive())
        .WillOnce(testing::Throw(std::runtime_error("Network error")));

    // Создаем сервер
    UdpServer server(*mockSessionManager,  "127.0.0.1", 8080, std::move(mockSocket));
    // Act
    server.start();
    // Assert
    EXPECT_NO_THROW(server.handler()); // Не должно падать при исключении
}