#include "mocks/logger.h"
#include "SessionManager.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>
#include <filesystem>
#include <utility> // Для std::move

// Фикстура для всех тестов
struct SessionManagerTest : public testing::Test {
    pgw::types::Blacklist blacklist {};
    pgw::types::Seconds timeout {2};
    pgw::types::Rate rate {30};
    const pgw::types::ConstImsi IMSI1 {"012340123401234"};
    const pgw::types::ConstImsi IMSI2 {"000111222333444"};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
    // Всегда удаляем файл после теста
    }
};

TEST_F(SessionManagerTest, CreateSessionManager){
    // Arrange
    // Act
    EXPECT_NO_THROW({
        SessionManager manager(blacklist, timeout, rate);
    });
    // Assert
}

TEST_F(SessionManagerTest, CreateSessionSucsess) {
    SessionManager manager {blacklist, timeout, rate};
    SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, SessionManager::CreateResult::CREATED);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.getSessionCount(), 1);
    result = manager.createSession(IMSI2);
    EXPECT_EQ(result, SessionManager::CreateResult::CREATED);
    EXPECT_TRUE(manager.hasSession(IMSI2));
    EXPECT_EQ(manager.getSessionCount(), 2);
}

TEST_F(SessionManagerTest, CreateSessionBlacklist) {
    auto imsi {IMSI1};
    blacklist.add(std::move(imsi));
    SessionManager manager {blacklist, timeout, rate};
    SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, SessionManager::CreateResult::REJECTED_BLACKLIST);
    EXPECT_FALSE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.getSessionCount(), 0);
}

TEST_F(SessionManagerTest, CreateSessionAlreadyExist) {
    SessionManager manager {blacklist, timeout, rate};
    manager.createSession(IMSI1);
    SessionManager::CreateResult result {manager.createSession(IMSI1)};
    EXPECT_EQ(result, SessionManager::CreateResult::ALREADY_EXISTS);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.getSessionCount(), 1);
}

TEST_F(SessionManagerTest, RemoveExistingSession) {
    SessionManager manager {blacklist, timeout, rate};
    manager.createSession(IMSI1);
    EXPECT_TRUE(manager.hasSession(IMSI1));
    manager.removeSession(IMSI1);
    EXPECT_FALSE(manager.hasSession(IMSI1));
    EXPECT_EQ(manager.getSessionCount(), 0);
}

TEST_F(SessionManagerTest, RemoveNonExistentSession) {
    SessionManager manager {blacklist, timeout, rate};
    manager.createSession(IMSI1);
    EXPECT_NO_THROW(manager.removeSession(IMSI2));
    EXPECT_EQ(manager.getSessionCount(), 1);
}

TEST_F(SessionManagerTest, CleanTimeoutSessionsNoneExpired) {
    SessionManager manager {blacklist, timeout, rate};
    manager.createSession(IMSI1);
    Session::сlock::time_point createdTime {Session::сlock::now()};
    manager.cleanTimeoutSessions();  // Создаем сессию и сразу очищаем - не должна удалиться
    EXPECT_EQ(manager.getSessionCount(), 1);
    while(std::chrono::duration_cast<std::chrono::seconds>(Session::сlock::now() - createdTime).count() < timeout){
        // Ждем, пока время жизни сессии не истечет
    }
    manager.cleanTimeoutSessions();
    EXPECT_EQ(manager.getSessionCount(), 0);
}

TEST_F(SessionManagerTest, GracefulShutdownSucsess) {
    SessionManager manager {blacklist, timeout, rate};
    SessionManager::CreateResult result;
    // Создаем 10 сессий
    for(int i = 0; i < 10; ++i) {
        std::string_view imsi {std::to_string(i)};
        result = manager.createSession(imsi);
        EXPECT_EQ(result, SessionManager::CreateResult::CREATED);
        EXPECT_TRUE(manager.hasSession(imsi));
        EXPECT_EQ(manager.getSessionCount(), i+1);
    }
    EXPECT_EQ(manager.getSessionCount(), 10);
    manager.gracefulShutdown(3); // Удаляем по 3 за раз
    EXPECT_EQ(manager.getSessionCount(), 7);
    manager.gracefulShutdown(3);
    EXPECT_EQ(manager.getSessionCount(), 4);
    manager.gracefulShutdown(3);
    EXPECT_EQ(manager.getSessionCount(), 1);
    manager.gracefulShutdown(3); // Случай сессий на удаление > актуальных сессий
    EXPECT_EQ(manager.getSessionCount(), 0);
}
