// Файлы mock директории
#include "logger.h"  // Логгер заглушка
#include "MockCdrWriter.h"

#include "SessionManager.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>
#include <filesystem>
#include <utility> // Для std::move

// Фикстура для всех тестов
struct SessionManagerTest : public testing::Test {
    std::unique_ptr<MockCdrWriter> mockCdrWriter;
    pgw::types::Blacklist blacklist {};
    pgw::types::seconds_t timeout {2};
    pgw::types::rate_t rate {30};
    const pgw::types::imsi_t IMSI1 {"012340123401234"};
    const pgw::types::imsi_t IMSI2 {"000111222333444"};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        mockCdrWriter = std::make_unique<MockCdrWriter>();
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        blacklist.clear();
    // Всегда удаляем файл после теста
    }
};

TEST_F(SessionManagerTest, CreateSessionManager){
    // Arrange
    // Act
    EXPECT_NO_THROW({
        SessionManager manager(*mockCdrWriter, blacklist, timeout, rate);
    });
    // Assert
}

TEST_F(SessionManagerTest, CreateSessionSucsess) {
    // Arrange
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, ICdrWriter::SESSION_CREATED))
        .Times(1);
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI2, ICdrWriter::SESSION_CREATED))
        .Times(1);

    SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, SessionManager::CreateResult::CREATED);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.countActiveSession(), 1);
    result = manager.createSession(IMSI2);
    EXPECT_EQ(result, SessionManager::CreateResult::CREATED);
    EXPECT_TRUE(manager.hasSession(IMSI2));
    EXPECT_EQ(manager.countActiveSession(), 2);
}

TEST_F(SessionManagerTest, CreateSessionBlacklist) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, ICdrWriter::SESSION_REJECTED))
        .Times(1);

    auto imsi {IMSI1};
    blacklist.add(std::move(imsi));
    SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, SessionManager::CreateResult::REJECTED_BLACKLIST);
    EXPECT_FALSE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.countActiveSession(), 0);
}

TEST_F(SessionManagerTest, CreateSessionAlreadyExist) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, ICdrWriter::SESSION_CREATED))
        .Times(1);

    SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    manager.createSession(IMSI1);
    SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, SessionManager::CreateResult::ALREADY_EXISTS);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.countActiveSession(), 1);
}

TEST_F(SessionManagerTest, RemoveExistingSession) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, ICdrWriter::SESSION_CREATED))
        .Times(1);
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, ICdrWriter::SESSION_DELETED))
        .Times(1);

    SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    manager.createSession(IMSI1);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    manager.terminateSession(IMSI1);
    EXPECT_FALSE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.countActiveSession(), 0);
}

TEST_F(SessionManagerTest, RemoveNonExistentSession) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, ICdrWriter::SESSION_CREATED))
        .Times(1);

    SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    manager.createSession(IMSI1);
    EXPECT_NO_THROW(manager.terminateSession(IMSI2));
    EXPECT_EQ(manager.countActiveSession(), 1);
}

TEST_F(SessionManagerTest, CleanTimeoutSessionsNoneExpired) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, ICdrWriter::SESSION_CREATED))
        .Times(1);
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, ICdrWriter::SESSION_DELETED))
        .Times(1);

    SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    manager.createSession(IMSI1);
    Session::clock::time_point createdTime {Session::clock::now()};
    manager.cleanTimeoutSessions();  // Создаем сессию и сразу очищаем - не должна удалиться
    EXPECT_EQ(manager.countActiveSession(), 1);
    while(std::chrono::duration_cast<std::chrono::seconds>(Session::clock::now() - createdTime).count() < timeout){
        // Ждем, пока время жизни сессии не истечет
    }
    manager.cleanTimeoutSessions();
    EXPECT_EQ(manager.countActiveSession(), 0);
}

TEST_F(SessionManagerTest, GracefulShutdownSucsess) {
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, ICdrWriter::SESSION_CREATED))
        .Times(10);
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, ICdrWriter::SESSION_DELETED))
        .Times(10);

    SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    SessionManager::CreateResult result;
    // Создаем 10 сессий
    for(int i = 0; i < 10; ++i) {
        std::string_view imsi {std::to_string(i)};
        result = manager.createSession(imsi);
        EXPECT_EQ(result, SessionManager::CreateResult::CREATED);
        EXPECT_TRUE(manager.hasSession(imsi));
        EXPECT_EQ(manager.countActiveSession(), i+1);
    }
    EXPECT_EQ(manager.countActiveSession(), 10);
    manager.gracefulShutdown(); // Удаляем сессии
    EXPECT_EQ(manager.countActiveSession(), 0);
}
