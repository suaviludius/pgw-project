#include "TcpHandler.h"
#include "TcpSerializer.h"
#include "MockSessionManager.h"
#include "MockCdrWriter.h"

#include <gtest/gtest.h>

// Фикстура для всех тестов
struct TcpHandlerTest : public testing::Test {

    // Набор заглушек
    std::unique_ptr<MockSessionManager> mockSessionManager;
    std::shared_ptr<MockCdrWriter> mockCdrWriter;
    // Набор тестовых констант
    const pgw::types::imsi_t IMSI1 {"123456789012345"};
    const pgw::types::imsi_t IMSI2 {"123456789012346"};
    // Набор тестовых переменных
    pgw::protocol::Message msg;
    // Флаг для проверки команды SHUTDOWN
    std::atomic<bool> shutdownRequest;
    // Сам TcpHandler
    std::unique_ptr<pgw::TcpHandler> tcpHandler;

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        mockSessionManager = std::make_unique<MockSessionManager>();
        mockCdrWriter = std::make_shared<MockCdrWriter>();
        shutdownRequest = false;
        tcpHandler = std::make_unique<pgw::TcpHandler>(*mockSessionManager,mockCdrWriter,shutdownRequest);

        msg.header.version = pgw::protocol::PROTOCOL_VERSION;
        msg.header.command = pgw::protocol::Command::TEST;
        msg.header.status = pgw::protocol::Status::TEST;
        msg.header.length = 0;
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
    }
};


TEST_F(TcpHandlerTest, GetStats_Success){
    // Arrange
    ISessionManager::Statistics testStatistic;
    testStatistic.activeSessions = 1,
    testStatistic.createdSessions = 2;
    testStatistic.rejectedSessions = 3;
    testStatistic.expiredSessions = 4;
    testStatistic.terminateSessions = 5;
    testStatistic.totalSessions = 6;
    testStatistic.uptimeSeconds = 7;

    EXPECT_CALL(*mockSessionManager, getStatistics())
        .Times(1)
        .WillOnce(testing::Return(testStatistic));  // При вызове mockSessionManager->getStatistics() вернём testStatistic

    // Заносим нужную команду в пакет
    msg.header.command = pgw::protocol::Command::GET_STATS;

    // Act
    auto responseMsg = tcpHandler->handle(msg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
    auto json = pgw::TcpSerializer::getJsonData(responseMsg);
    EXPECT_EQ(json["active_sessions"], testStatistic.activeSessions);
    EXPECT_EQ(json["created_sessions"], testStatistic.createdSessions);
    EXPECT_EQ(json["expired_sessions"], testStatistic.expiredSessions);
    EXPECT_EQ(json["rejected_sessions"], testStatistic.rejectedSessions);
    EXPECT_EQ(json["terminate_sessions"], testStatistic.terminateSessions);
    EXPECT_EQ(json["total_sessions"], testStatistic.totalSessions);
    EXPECT_EQ(json["uptime_seconds"], testStatistic.uptimeSeconds);
}

TEST_F(TcpHandlerTest, GetSession_Success){
    // Arrange
    ISessionManager::SessionData sessionData {std::chrono::steady_clock::now()};
    ISessionManager::sessions testSessions;
    testSessions[IMSI1] = sessionData;

    EXPECT_CALL(*mockSessionManager, getActiveSessions())
        .Times(1)
        .WillOnce(testing::Return(testSessions));  // При вызове mockSessionManager->getStatistics() вернём testStatistic

    // Заносим нужную команду в пакет
    msg.header.command = pgw::protocol::Command::GET_SESSIONS;

    // Act
    auto responseMsg = tcpHandler->handle(msg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
    auto json = pgw::TcpSerializer::getJsonData(responseMsg);
    ASSERT_TRUE(json["sessions"].is_array());
    const auto& firstSession = json["sessions"][0];
    EXPECT_EQ(firstSession["imsi"], IMSI1);
    EXPECT_EQ(json["count"], 1);
}

// TODO: сейчас интерфейс базы данных слишком связан с CDR классом,
// необходимо в интерфейсе сделать универсальные методы добавления в таблицу и
// чтения записей из таблицы. Также вынести CdrRecord в ICdrWriter

TEST_F(TcpHandlerTest, GetCdr_Sucsess){
    // Arrange
    constexpr size_t limit = 10;

    std::vector<pgw::CdrRecord> records = {
        {IMSI1, "start", "2024-01-01 10:00:00"},
        {IMSI1, "stop", "2024-01-01 10:05:00"}
    };

    EXPECT_CALL(*mockCdrWriter, getRecentRecords(limit))
        .Times(1)
        .WillOnce(testing::Return(records));

    auto jsonMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::GET_CDR,
        pgw::protocol::Status::TEST,
        {{"limit", limit}}
    );

    // Act
    auto responseMsg = tcpHandler->handle(jsonMsg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
    auto json = pgw::TcpSerializer::getJsonData(responseMsg);
    ASSERT_TRUE(json["records"].is_array());
    // Тут стоит учитывать, что нет гарантий того, что данные в json
    // будут находится в том порядке, в каком мы иъ записывали,
    // поэтому, чтобы не усложнять тест, сделаем мнимые сравнения
    const auto& firstSession = json["records"][0];
    const auto& secondSession = json["records"][1];
    EXPECT_EQ(firstSession["imsi"], secondSession["imsi"]);
    EXPECT_NE(firstSession["action"], secondSession["action"]);
    EXPECT_NE(firstSession["timestamp"], secondSession["timestamp"]);
    EXPECT_EQ(json["count"], 2);
    EXPECT_EQ(json["limit"], limit);
}

TEST_F(TcpHandlerTest, StartSession_Success){
    // Arrange
    EXPECT_CALL(*mockSessionManager, createSession(IMSI1))
        .Times(1)
        .WillOnce(testing::Return(ISessionManager::CreateResult::CREATED));

    auto jsonMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::START_SESSION,
        pgw::protocol::Status::TEST,
        {{"imsi", IMSI1}}
    );

    // Act
    auto responseMsg = tcpHandler->handle(jsonMsg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
    auto json = pgw::TcpSerializer::getJsonData(responseMsg);
    EXPECT_EQ(json["result"], "created");
    EXPECT_EQ(json["status"], "success");
}

TEST_F(TcpHandlerTest, StartSession_AlreadyExists) {
    // Arrange
    EXPECT_CALL(*mockSessionManager, createSession(IMSI1))
        .Times(1)
        .WillOnce(testing::Return(ISessionManager::CreateResult::ALREADY_EXISTS));

    auto jsonMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::START_SESSION,
        pgw::protocol::Status::TEST,
        {{"imsi", IMSI1}}
    );

    // Act
    auto responseMsg = tcpHandler->handle(jsonMsg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
    auto json = pgw::TcpSerializer::getJsonData(responseMsg);
    EXPECT_EQ(json["result"], "exists");
    EXPECT_EQ(json["status"], "failed");
}

TEST_F(TcpHandlerTest, StartSession_RejectedBlacklist) {
    // Arrange
    EXPECT_CALL(*mockSessionManager, createSession(IMSI1))
        .Times(1)
        .WillOnce(testing::Return(ISessionManager::CreateResult::REJECTED_BLACKLIST));

    auto jsonMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::START_SESSION,
        pgw::protocol::Status::TEST,
        {{"imsi", IMSI1}}
    );

    // Act
    auto responseMsg = tcpHandler->handle(jsonMsg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
    auto json = pgw::TcpSerializer::getJsonData(responseMsg);
    EXPECT_EQ(json["result"], "rejected");
    EXPECT_EQ(json["status"], "failed");
}

TEST_F(TcpHandlerTest, StartSession_InvalidParams) {
    // Arrange
    EXPECT_CALL(*mockSessionManager, createSession("7"))
        .Times(0);

    auto jsonMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::START_SESSION,
        pgw::protocol::Status::TEST,
        {{"imsi", "7"}}
    );

    // Act
    auto responseMsg = tcpHandler->handle(jsonMsg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::INVALID_PARAMS);
}

TEST_F(TcpHandlerTest, StopSession_Success) {
    // Arrange
    EXPECT_CALL(*mockSessionManager, terminateSession(IMSI1))
        .Times(1)
        .WillOnce(testing::Return(true));

    auto jsonMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::STOP_SESSION,
        pgw::protocol::Status::TEST,
        {{"imsi", IMSI1}}
    );

    // Act
    auto responseMsg = tcpHandler->handle(jsonMsg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
    auto json = pgw::TcpSerializer::getJsonData(responseMsg);
    EXPECT_EQ(json["result"], "deleted");
    EXPECT_EQ(json["status"], "success");
}


TEST_F(TcpHandlerTest, StopSession_NotFound) {
    // Arrange
    EXPECT_CALL(*mockSessionManager, terminateSession(IMSI1))
        .Times(1)
        .WillOnce(testing::Return(false));

    auto jsonMsg = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::STOP_SESSION,
        pgw::protocol::Status::TEST,
        {{"imsi", IMSI1}}
    );

    // Act
    auto responseMsg = tcpHandler->handle(jsonMsg);

    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
    auto json = pgw::TcpSerializer::getJsonData(responseMsg);
    EXPECT_EQ(json["result"], "not_found");
    EXPECT_EQ(json["status"], "failed");
}

TEST_F(TcpHandlerTest, Shutdown_Success) {
    // Arrange
    msg.header.command = pgw::protocol::Command::SHUTDOWN;

    // Act
    auto responseMsg = tcpHandler->handle(msg);

    // Assert
    EXPECT_TRUE(shutdownRequest.load());
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::OK);
}

TEST_F(TcpHandlerTest, InvalidCommand_Success) {
    // Arrange
    msg.header.command = pgw::protocol::Command::TEST;
    // Act
    auto responseMsg = tcpHandler->handle(msg);
    // Assert
    EXPECT_EQ(responseMsg.header.status, pgw::protocol::Status::INVALID_COMMAND);
}
