#include "config.h"
#include "SessionManager.h"

#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>

// Фикстура для всех тестов
struct SessionManagerTest : public testing::Test {
    // static constexpr const char* CONFIG_FILE {"test_config.json"};
    const pgw::types::ConstImsi IMSI1 {"012340123401234"};
    const pgw::types::ConstImsi IMSI2 {"000111222333444"};
    SessionManager manager;

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){
    }
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuit(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        Config config(""); // Будем использовать default значения для тестирования
        auto manager {std::move(SessionManager(config))};
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // Всегда удаляем файл после теста
        // if (std::filesystem::exists(CONFIG_FILE)) {
        //     std::filesystem::remove(CONFIG_FILE);
        // }
    }
};

TEST_F(SessionManagerTest, CreateSession){
    // Arrange
    SessionManager::CreateResult createResult;
    size_t sessionCount {manager.getSessionCount()};
    // Act
    EXPECT_NO_THROW(createResult = manager.createSession(IMSI1));
    // Assert
    EXPECT_TRUE(createResult == SessionManager::CreateResult::CREATED);
    EXPECT_TRUE((sessionCount + 1) == manager.getSessionCount());
}