#include "logger.h"

#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>

// Фикстура для всех тестов
struct LoggerTest : public testing::Test {
    static constexpr const char* LOGGER_FILE {"test_logger.log"};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){
    }
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuit(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        // Убедимся, что файла нет
        if (std::filesystem::exists(LOGGER_FILE)) {
            std::filesystem::remove(LOGGER_FILE);
        }
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // Всегда удаляем файл после теста
        if (std::filesystem::exists(LOGGER_FILE)) {
            std::filesystem::remove(LOGGER_FILE);
        }
    }

    bool findLoggerContent(std::string_view content) {
        std::ifstream file(LOGGER_FILE);
        std::string line;
        if (!file) {
            throw std::runtime_error("Cannot open test logger file");
        }
        while (std::getline(file, line)) {
            if (line.find(content) != std::string::npos) {
                return true;
            }
        }
        return false;
        // file.close() автоматически в деструкто
    }

    bool loggerFileExists() const {
        return std::filesystem::exists(LOGGER_FILE);
    }

};

TEST_F(LoggerTest, CreatesLogFile){
    // Arrange
    std::string_view message {"Test message"};
    std::string_view level {"INFO"};
    // Act
    EXPECT_NO_THROW({
        pgw::logger::init(LOGGER_FILE, level);
        LOG_INFO(message);
        pgw::logger::shutdown();
    });
    // Assert
    ASSERT_TRUE(loggerFileExists());
    EXPECT_TRUE(findLoggerContent(message));
}

TEST_F(LoggerTest, LogLevelOperation){
    // Arrange
    // Act
    EXPECT_NO_THROW({
        pgw::logger::init(LOGGER_FILE, "WARN");
        LOG_INFO("Test message");
        pgw::logger::shutdown();
    });
    // Assert
    ASSERT_TRUE(loggerFileExists());
    EXPECT_FALSE(findLoggerContent("Test message"));
}

TEST_F(LoggerTest, IncorrectLogLevel){
    // Arrange
    // Act
    // Assert
    EXPECT_ANY_THROW(pgw::logger::init(LOGGER_FILE, "TEST"));
    pgw::logger::shutdown();
}