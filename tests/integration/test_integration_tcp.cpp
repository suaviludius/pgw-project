#include "Server.h"

#include "pgw/common/SocketFactory.h"
#include "pgw/common/protocol.h"
#include "pgw/common/TcpSerializer.h"
#include "pgw/common/logger.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <future>
#include <poll.h>

struct IntegrationTcpTest : public testing::Test {
    // Набор путей к файлам
    static constexpr const char* CLIENT_CONFIG_FILE {"test_config_pgw_client.json"}; // или настоящий: "configs/pgw_client.json"
    static constexpr const char* SERVER_CONFIG_FILE {"test_config_pgw_server.json"}; // или настоящий: "configs/pgw_server.json"
    static constexpr const char* CDR_FILE {"test_integration_cdr.log"};
    // Программа не расчитана на то, что будет создаваться несколько логеров,
    // поэтому объявим один и будем ловить ошибки повторной инициализации
    static constexpr const char* LOG_FILE {"test_integration.log"};
    static constexpr const char* DB_FILE {"test_integration.db"};
    // Набор тестовых констант
    const pgw::types::imsi_t IMSI1 {"000000000000001"};
    const pgw::types::imsi_t IMSI2 {"000000000000002"};
    const pgw::types::imsi_t IMSI3 {"000000000000003"};
    const pgw::types::imsi_t IMSI4 {"000000000000004"};
    const pgw::types::imsi_t IMSI5 {"000000000000005"};
    const pgw::types::imsi_t IMSI6 {"000000000000006"};
    const pgw::types::imsi_t IMSI_BLACKLISTED {"101010101010101"};
    const pgw::types::seconds_t SESSION_TIMEOUT_S {3}; // Создаем свой таймаут для сессии, чтобы долго не ждать

    const int CLIENT_READY_TIMEOUT_MS {5000}; // Таймаут для poll() ожидания выполнения команд клиента

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuite(){
        createTestConfigs();
    }
    // Метод, вызываемый после всех тестов
    static void TearDownTestSuite(){
        std::filesystem::remove(CLIENT_CONFIG_FILE);
        std::filesystem::remove(SERVER_CONFIG_FILE);
    }
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {}
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // Удаляем тестовые файлы
        std::filesystem::remove(CDR_FILE);
        std::filesystem::remove(LOG_FILE);
        std::filesystem::remove(DB_FILE);
    }

    // Создадим файлы тетсовых конфигов для проверки
    static void createTestConfigs() {
        std::ofstream serverConfig(SERVER_CONFIG_FILE);
        if (!serverConfig) {
            throw std::runtime_error("Cannot create server test config file");
        }
        serverConfig << R"({
            "udp_port": 9000,
            "tcp_ip": "0.0.0.0",
            "tcp_port": 9090,
            "session_timeout_sec": 30,
            "database_file": ")" << DB_FILE << R"(",
            "cdr_file": ")" << CDR_FILE << R"(",
            "http_port": 8080,
            "graceful_shutdown_rate": 10,
            "log_file": ")" << LOG_FILE << R"(",
            "log_level": "DEBUG",
            "blacklist": ["101010101010101"]
        })";
        serverConfig.close();

    }


    // Функция как TcpServer::handleClientData() но попроще
    std::vector<uint8_t> readMessgae(pgw::ITcpSocket& socket){

        // Буффер для сохранения прочитанных данных
        std::vector<uint8_t> buffer;
        // Начало ожидание
        const auto start = std::chrono::steady_clock::now();
        // Длительность ожидания
        const int wait = 10;

        while (true) {
            // Будем давать время на чтение, чтобы не прервыать тест самому
            if (std::chrono::steady_clock::now() - start > std::chrono::seconds(wait)) {
                throw std::runtime_error("Timeout waiting for message");
            }
            auto packet = socket.receive();

            if(!packet.data.empty()) {
                LOG_DEBUG("Received {} bytes, buffer size now {}", packet.data.size(), buffer.size());
                buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());

                // Считываем первое сообщение из буффера
                auto msg = pgw::TcpSerializer::deserializer(buffer);
                if (msg.has_value()) {
                    return buffer; // Достаточно данных для полного сообщения
                }

            }

            // Небольшая задержка перед следующим чтением
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        }
    }
};

TEST_F(IntegrationTcpTest, TcpCommandIndividual) {

    // Arrange 1
    // Сервер
    pgw::server::Server server(SERVER_CONFIG_FILE);
    auto serverFuture = std::async(std::launch::async, [&server](){
        // Act
        return server.run();
    });

    // Даем время серверу на проснуться
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Менеджер сессий для проверок
    auto& sm = server.getSessionManager();

    // Клиент
    auto tcpClient = pgw::SocketFactory::createTcp(false);

    // Assert 1
    EXPECT_NE(tcpClient, nullptr);
    ASSERT_FALSE(tcpClient->isListening());
    ASSERT_GT(tcpClient->getFd(), 0);

    tcpClient->connect("127.0.0.1", 9090);

    pollfd pfd = {tcpClient->getFd(), POLLOUT, 0};
    int ready = poll(&pfd, 1, CLIENT_READY_TIMEOUT_MS);
    ASSERT_GT(ready,0);

    // Arrange 2 - START_SESSION
    {
        auto requestMsg = pgw::TcpSerializer::createJsonMsg(
            pgw::protocol::Command::START_SESSION,
            pgw::protocol::Status::TEST,
            {{"imsi", IMSI1}}
        );
        auto request = pgw::TcpSerializer::serializer(requestMsg);

        // Act 2
        tcpClient->send(std::string(request.begin(), request.end()));
        auto response = readMessgae(*tcpClient);

        // Assert 2
        ASSERT_FALSE(response.empty());

        auto responseMsg = pgw::TcpSerializer::deserializer(response);
        ASSERT_TRUE(responseMsg.has_value());
        EXPECT_EQ(responseMsg->header.status, pgw::protocol::Status::OK);

        auto responseJson = pgw::TcpSerializer::getJsonData(*responseMsg);
        EXPECT_EQ(responseJson["result"], "created");

        EXPECT_TRUE(sm.hasSession(IMSI1));
        EXPECT_EQ(sm.countActiveSession(), 1);
    }

    // Arrange 3 - GET_SESSIONS
    {
        pgw::protocol::Message requestMsg;
        requestMsg.header.version = pgw::protocol::PROTOCOL_VERSION;
        requestMsg.header.command = pgw::protocol::Command::GET_SESSIONS;
        requestMsg.header.length = 0;
        auto request = pgw::TcpSerializer::serializer(requestMsg);

        // Act 3
        tcpClient->send(std::string(request.begin(), request.end()));
        auto response = readMessgae(*tcpClient);

        // Assert 3
        ASSERT_FALSE(response.empty());

        auto responseMsg = pgw::TcpSerializer::deserializer(response);
        ASSERT_TRUE(responseMsg.has_value());
        EXPECT_EQ(responseMsg->header.status, pgw::protocol::Status::OK);

        auto responseJson = pgw::TcpSerializer::getJsonData(*responseMsg);
        EXPECT_EQ(responseJson["count"], 1);
        EXPECT_EQ(responseJson["sessions"][0]["imsi"], IMSI1);
    }

    // Arrange 4 - STOP_SESSION
    {
        auto requestMsg = pgw::TcpSerializer::createJsonMsg(
            pgw::protocol::Command::STOP_SESSION,
            pgw::protocol::Status::TEST,
            {{"imsi", IMSI1}}
        );
        auto request = pgw::TcpSerializer::serializer(requestMsg);

        // Act 4
        tcpClient->send(std::string(request.begin(), request.end()));
        auto response = readMessgae(*tcpClient);

        // Assert 4
        ASSERT_FALSE(response.empty());

        auto responseMsg = pgw::TcpSerializer::deserializer(response);
        ASSERT_TRUE(responseMsg.has_value());
        EXPECT_EQ(responseMsg->header.status, pgw::protocol::Status::OK);

        EXPECT_FALSE(sm.hasSession(IMSI1));
        EXPECT_EQ(sm.countActiveSession(), 0);

    }

    // Arrange 5 - GET_STAT
    {
        auto requestMsg = pgw::TcpSerializer::createJsonMsg(
            pgw::protocol::Command::GET_STATS,
            pgw::protocol::Status::TEST
        );
        auto request = pgw::TcpSerializer::serializer(requestMsg);

        // Act 5
        tcpClient->send(std::string(request.begin(), request.end()));
        auto response = readMessgae(*tcpClient);

        // Assert 5
        ASSERT_FALSE(response.empty());

        auto responseMsg = pgw::TcpSerializer::deserializer(response);
        ASSERT_TRUE(responseMsg.has_value());
        EXPECT_EQ(responseMsg->header.status, pgw::protocol::Status::OK);

        auto responseJson = pgw::TcpSerializer::getJsonData(*responseMsg);
        EXPECT_EQ(responseJson["active_sessions"], 0);
        EXPECT_EQ(responseJson["created_sessions"], 1);
        EXPECT_EQ(responseJson["expired_sessions"], 0);
        EXPECT_EQ(responseJson["rejected_sessions"], 0);
        EXPECT_EQ(responseJson["terminate_sessions"], 1);
        EXPECT_EQ(responseJson["total_sessions"], 1);
    }

    // Arrange 5 - SHUTDOWN
    {
        pgw::protocol::Message requestMsg;
        requestMsg.header.version = pgw::protocol::PROTOCOL_VERSION;
        requestMsg.header.command = pgw::protocol::Command::SHUTDOWN;
        requestMsg.header.length = 0;
        auto request = pgw::TcpSerializer::serializer(requestMsg);

        // Act 5
        tcpClient->send(std::string(request.begin(), request.end()));

        // Просто ждём немного
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Assert 5
    EXPECT_EQ(serverFuture.get(), 0);

    tcpClient->close();
}


TEST_F(IntegrationTcpTest, TcpCommandParallel) {

    // Arrange 1
    // Сервер
    pgw::server::Server server(SERVER_CONFIG_FILE);
    auto serverFuture = std::async(std::launch::async, [&server](){
        // Act
        return server.run();
    });

    // Даем время серверу на проснуться
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Менеджер сессий для проверок
    auto& sm = server.getSessionManager();

    // Лямбда для одного клиента (без ASSERT, т.к. они не работают в потоках)
    auto clientTask = [&](const pgw::types::imsi_t& imsi) -> bool {
        try {
            auto client = pgw::SocketFactory::createTcp(false);
            client->connect("127.0.0.1", 9090);

            // Ждем готовности сокета к записи
            pollfd pfd = {client->getFd(), POLLOUT, 0};
            int ready = poll(&pfd, 1, 5000);
            if (ready <= 0) return false;

            // START_SESSION
            auto startMsg = pgw::TcpSerializer::createJsonMsg(
                pgw::protocol::Command::START_SESSION,
                pgw::protocol::Status::TEST,
                {{"imsi", imsi}}
            );
            auto startRequest = pgw::TcpSerializer::serializer(startMsg);

            client->send(std::string(startRequest.begin(), startRequest.end()));
            auto response = readMessgae(*client);

            if (response.empty()) return false;
            auto responseMsg = pgw::TcpSerializer::deserializer(response);
            if (!responseMsg.has_value()) return false;
            if (responseMsg->header.status != pgw::protocol::Status::OK) return false;

            // STOP_SESSION
            auto stopMsg = pgw::TcpSerializer::createJsonMsg(
                pgw::protocol::Command::STOP_SESSION,
                pgw::protocol::Status::TEST,
                {{"imsi", imsi}}
            );
            auto stopRequest = pgw::TcpSerializer::serializer(stopMsg);

            client->send(std::string(stopRequest.begin(), stopRequest.end()));
            response = readMessgae(*client);

            if (response.empty()) return false;
            responseMsg = pgw::TcpSerializer::deserializer(response);
            if (!responseMsg.has_value()) return false;

            client->close();
            return responseMsg->header.status == pgw::protocol::Status::OK;

        } catch (...) {
            return false;
        }
    };

    // Arrange 2
    // Act 2
    auto future1 = std::async(std::launch::async, clientTask, IMSI1);
    auto future2 = std::async(std::launch::async, clientTask, IMSI2);
    auto future3 = std::async(std::launch::async, clientTask, IMSI3);

    // Assert 2
    EXPECT_TRUE(future1.get());
    EXPECT_TRUE(future2.get());
    EXPECT_TRUE(future3.get());

    // Финальные проверки в основном потоке
    EXPECT_FALSE(sm.hasSession(IMSI1));
    EXPECT_FALSE(sm.hasSession(IMSI2));
    EXPECT_FALSE(sm.hasSession(IMSI3));
    EXPECT_EQ(sm.countActiveSession(), 0);

    // Act 3
    server.stop();

    // Assert 3
    EXPECT_EQ(serverFuture.get(), 0);
}