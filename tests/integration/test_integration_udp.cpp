#include "Client.h"
#include "Server.h"

#include <gtest/gtest.h>

#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <future>


struct IntegrationTest : public testing::Test {
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

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuite(){
        createTestConfigs();
    }
    // Метод, вызываемый после всех тестов
    static void TearDownTestSuite(){
        std::filesystem::remove(CLIENT_CONFIG_FILE);
        std::filesystem::remove(SERVER_CONFIG_FILE);
        std::filesystem::remove(CDR_FILE);
        std::filesystem::remove(LOG_FILE);
        std::filesystem::remove(DB_FILE);
    }
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {}
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // Удаляем тестовые файлы
        std::filesystem::remove(CLIENT_CONFIG_FILE);
        std::filesystem::remove(SERVER_CONFIG_FILE);
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
            "udp_ip": "0.0.0.0",
            "udp_port": 9000,
            "tcp_ip": "0.0.0.0",
            "tcp_port": 9090,
            "session_timeout_sec": 30,
            "database_file": ")" << DB_FILE << R"(",
            "cdr_file": ")" << CDR_FILE << R"(",
            "http_port": 8080,
            "graceful_shutdown_rate": 10,
            "log_file": ")" << LOG_FILE << R"(",
            "log_level": "INFO",
            "blacklist": ["101010101010101"]
        })";
        serverConfig.close();

        std::ofstream clientConfig(CLIENT_CONFIG_FILE);
        if (!clientConfig) {
            throw std::runtime_error("Cannot create client test config file");
        }
        clientConfig << R"({
            "server_ip": "127.0.0.1",
            "server_port": 9000,
            "log_file": ")" << LOG_FILE << R"(",
            "log_level": "INFO"
        })";
        clientConfig.close();
    }
};

TEST_F(IntegrationTest, FullUdpWork) {

    // Arrange
    pgw::server::Server server(SERVER_CONFIG_FILE);
    auto serverFuture = std::async(std::launch::async, [&server](){
        // Act
        return server.run();
    });

    // Даем время серверу на проснуться
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Act 1
    pgw::client::Client client1(CLIENT_CONFIG_FILE,IMSI1);
    auto client1Result = client1.run();

    //TODO: нужно исправить, что ответ не представлен в виде структуры,
    // а просто магическая строка "created"

    // Assert 1
    EXPECT_EQ(client1Result, 0);
    EXPECT_EQ(client1.getResponse(), "created");

    auto& serverSessionManager = server.getSessionManager();
    EXPECT_TRUE(serverSessionManager.hasSession(IMSI1));
    EXPECT_EQ(serverSessionManager.countActiveSession(), 1);

    // Act 2
    pgw::client::Client client2(CLIENT_CONFIG_FILE, IMSI_BLACKLISTED);
    int client2Result = client2.run();

    // Assert 2
    EXPECT_EQ(client2Result, 0);
    EXPECT_EQ(client2.getResponse(), "rejected");

    EXPECT_FALSE(serverSessionManager.hasSession(IMSI_BLACKLISTED));
    EXPECT_EQ(serverSessionManager.countActiveSession(), 1);


    // Arrange 3
    std::vector<pgw::types::imsi_t> imsis = {IMSI2, IMSI3, IMSI4};
    // Act 3
    for (const auto& imsi : imsis) {
        pgw::client::Client client(CLIENT_CONFIG_FILE, imsi);
        client.run();
        EXPECT_TRUE(serverSessionManager.hasSession(imsi));
    }
    // Assert 3
    EXPECT_EQ(serverSessionManager.countActiveSession(), 4);

    // Arrange 4
    EXPECT_TRUE(serverSessionManager.terminateSession(IMSI2));
    EXPECT_TRUE(serverSessionManager.terminateSession(IMSI3));
    EXPECT_TRUE(serverSessionManager.terminateSession(IMSI4));
    EXPECT_EQ(serverSessionManager.countActiveSession(), 1);
    std::vector<std::future<int>> clientsFutures;
    for (const auto& imsi : imsis) {
        clientsFutures.push_back(std::async(std::launch::async, [this, imsi]() {
            pgw::client::Client client(CLIENT_CONFIG_FILE, imsi);
            return client.run();
        }));
    }
    // Act 4
    for (auto& future : clientsFutures) {
        EXPECT_EQ(future.get(), 0);
    }
    // Assert 4
    for (const auto& imsi : imsis) {
        EXPECT_TRUE(serverSessionManager.hasSession(imsi));
    }
    EXPECT_EQ(serverSessionManager.countActiveSession(), imsis.size() + 1);

    // Act 5
    server.stop();
    // Assert 5
    EXPECT_EQ(serverFuture.get(), 0);

    // Act 6
    auto records = server.getCdrWriter()->getRecentRecords(10);
    // Assert 6
    EXPECT_GE(records.size(), 5);  // created для IMSI1 + IMSI2,3,4 + rejected для blacklisted + ...
}

