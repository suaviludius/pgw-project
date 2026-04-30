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
        pgw::SessionManager manager(*mockCdrWriter, blacklist, timeout, rate);
    });
    // Assert
}

TEST_F(SessionManagerTest, CreateSessionSucsess) {
    // Arrange
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_CREATED))
        .Times(1);
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI2, pgw::ICdrWriter::SESSION_CREATED))
        .Times(1);

    pgw::SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    pgw::SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, pgw::SessionManager::CreateResult::CREATED);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.countActiveSession(), 1);
    result = manager.createSession(IMSI2);
    EXPECT_EQ(result, pgw::SessionManager::CreateResult::CREATED);
    EXPECT_TRUE(manager.hasSession(IMSI2));
    EXPECT_EQ(manager.countActiveSession(), 2);
}

TEST_F(SessionManagerTest, CreateSessionBlacklist) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_REJECTED))
        .Times(1);

    auto imsi {IMSI1};
    blacklist.emplace(std::move(imsi));
    pgw::SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    pgw::SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, pgw::SessionManager::CreateResult::REJECTED_BLACKLIST);
    EXPECT_FALSE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.countActiveSession(), 0);
}

TEST_F(SessionManagerTest, CreateSessionAlreadyExist) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_CREATED))
        .Times(1);

    pgw::SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    manager.createSession(IMSI1);
    pgw::SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, pgw::SessionManager::CreateResult::ALREADY_EXISTS);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.countActiveSession(), 1);
}

TEST_F(SessionManagerTest, RemoveExistingSession) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_CREATED))
        .Times(1);
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_DELETED))
        .Times(1);

    pgw::SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    manager.createSession(IMSI1);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    manager.terminateSession(IMSI1);
    EXPECT_FALSE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.countActiveSession(), 0);
}

TEST_F(SessionManagerTest, RemoveNonExistentSession) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_CREATED))
        .Times(1);

    pgw::SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    manager.createSession(IMSI1);
    EXPECT_NO_THROW(manager.terminateSession(IMSI2));
    EXPECT_EQ(manager.countActiveSession(), 1);
}

TEST_F(SessionManagerTest, CleanTimeoutSessionsNoneExpired) {
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_CREATED))
        .Times(1);
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_DELETED))
        .Times(1);

    pgw::SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    manager.createSession(IMSI1);
    std::chrono::steady_clock::time_point createdTime {std::chrono::steady_clock::now()};
    manager.cleanTimeoutSessions();  // Создаем сессию и сразу очищаем - не должна удалиться
    EXPECT_EQ(manager.countActiveSession(), 1);
    while((std::chrono::steady_clock::now() - createdTime) < timeout){
        // Ждем, пока время жизни сессии не истечет
    }
    manager.cleanTimeoutSessions();
    EXPECT_EQ(manager.countActiveSession(), 0);
}

TEST_F(SessionManagerTest, GracefulShutdownSucsess) {
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, pgw::ICdrWriter::SESSION_CREATED))
        .Times(10);
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, pgw::ICdrWriter::SESSION_DELETED))
        .Times(10);

    pgw::SessionManager manager {*mockCdrWriter, blacklist, timeout, rate};
    pgw::SessionManager::CreateResult result;

    // Создаем 10 сессий
    for(int i = 0; i < 10; ++i) {
        std::string imsi {std::to_string(i)};
        result = manager.createSession(imsi);
        EXPECT_EQ(result, pgw::SessionManager::CreateResult::CREATED);
        EXPECT_TRUE(manager.hasSession(imsi));
        EXPECT_EQ(manager.countActiveSession(), i+1);
    }

    EXPECT_EQ(manager.countActiveSession(), 10);

    manager.gracefulShutdown(); // Удаляем сессии

    EXPECT_EQ(manager.countActiveSession(), 0);
}


TEST_F(SessionManagerTest, GetStatistics) {
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, pgw::ICdrWriter::SESSION_CREATED))
    .Times(2);
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, pgw::ICdrWriter::SESSION_DELETED))
    .Times(1);
    pgw::SessionManager manager(*mockCdrWriter, blacklist, timeout, rate);

    auto stats = manager.getStatistics();
    EXPECT_EQ(stats.createdSessions, 0);
    EXPECT_EQ(stats.activeSessions, 0);

    manager.createSession(IMSI1);
    manager.createSession(IMSI2);
    manager.terminateSession(IMSI1);

    stats = manager.getStatistics();
    EXPECT_EQ(stats.createdSessions, 2);
    EXPECT_EQ(stats.activeSessions, 1);
    EXPECT_EQ(stats.terminateSessions, 1);
    EXPECT_EQ(stats.expiredSessions, 0);
}

TEST_F(SessionManagerTest, GetShutdownStatistics) {
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, pgw::ICdrWriter::SESSION_CREATED))
    .Times(2);
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, pgw::ICdrWriter::SESSION_DELETED))
    .Times(2);
    pgw::SessionManager manager(*mockCdrWriter, blacklist, timeout, rate);

    auto stats = manager.getStatistics();
    EXPECT_EQ(stats.createdSessions, 0);
    EXPECT_EQ(stats.activeSessions, 0);

    manager.createSession(IMSI1);
    manager.createSession(IMSI2);

    manager.gracefulShutdown();

    stats = manager.getStatistics();
    EXPECT_EQ(stats.terminateSessions, 2);
    EXPECT_EQ(stats.activeSessions, 0);
}

TEST_F(SessionManagerTest, GetTimeoutStatistics) {
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, pgw::ICdrWriter::SESSION_CREATED))
    .Times(2);
    EXPECT_CALL(*mockCdrWriter, writeAction(testing::_, pgw::ICdrWriter::SESSION_DELETED))
    .Times(2);
    pgw::SessionManager manager(*mockCdrWriter, blacklist, timeout, rate);

    auto stats = manager.getStatistics();
    EXPECT_EQ(stats.createdSessions, 0);
    EXPECT_EQ(stats.activeSessions, 0);

    manager.createSession(IMSI1);
    manager.createSession(IMSI2);

    // Дождемся таймаута сессии и проверим статистику
    std::chrono::steady_clock::time_point createdTime {std::chrono::steady_clock::now()};
    while((std::chrono::steady_clock::now() - createdTime) < timeout){
        // Ждем, пока время жизни сессий не истечет
    }
    manager.cleanTimeoutSessions();

    stats = manager.getStatistics();
    EXPECT_EQ(stats.expiredSessions, 2);
    EXPECT_GE(stats.uptimeSeconds, timeout.count());
}

TEST_F(SessionManagerTest, AddToBlacklist) {
    pgw::SessionManager manager(*mockCdrWriter, blacklist, timeout, rate);

    EXPECT_TRUE(manager.addToBlacklist(IMSI1));
    EXPECT_FALSE(manager.addToBlacklist(IMSI1)); // Да-да, еще разочек
    EXPECT_FALSE(manager.addToBlacklist("1234")); // Что будет, если дать неправильный imsi?

    // Проверка, что blacklist работает
    EXPECT_CALL(*mockCdrWriter, writeAction(IMSI1, pgw::ICdrWriter::SESSION_REJECTED));
    auto result = manager.createSession(IMSI1);
    EXPECT_EQ(result, pgw::SessionManager::CreateResult::REJECTED_BLACKLIST);
}

TEST_F(SessionManagerTest, GracefulShutdownEmpty) {
    pgw::SessionManager manager(*mockCdrWriter, blacklist, timeout, rate);
    EXPECT_NO_THROW(manager.gracefulShutdown());
}