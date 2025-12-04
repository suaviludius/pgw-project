#include "config.h"
#include "constants.h"

#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>

// Фикстура для всех тестов
struct ConfigTest : public testing::Test {
    static constexpr const char* CONFIG_FILE {"test_config.json"};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){
    }
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuit(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        // Убедимся, что файла нет
        if (std::filesystem::exists(CONFIG_FILE)) {
            std::filesystem::remove(CONFIG_FILE);
        }
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // Всегда удаляем файл после теста
        if (std::filesystem::exists(CONFIG_FILE)) {
            std::filesystem::remove(CONFIG_FILE);
        }
    }

    void createTestConfig(const std::string& content) {
        std::ofstream file(CONFIG_FILE);
        if (!file) {
            throw std::runtime_error("Cannot create test config file");
        }
        file << content;
        file.close(); // Автоматически в деструкторе
    }

    bool configFileExists() const {
        return std::filesystem::exists(CONFIG_FILE);
    }

};

// + Отсутствующий файл
// + Успешное чтение
// + Неполные данные (значения по умолчанию)
// + Невалидне данные JSON
// - Неправильные типы данных
// - Граничные значения (порты 0, 65535 и т.д.)

TEST_F(ConfigTest, MissingFile){
    // Arrange
    // Act
    // Assert
    EXPECT_NO_THROW({
        Config config("non_existent_file.json");
    });
}

TEST_F(ConfigTest, SuccessfulReading) {
    // Arrange
    // R(raw string - спецсимволы не обрабатываются)
    createTestConfig( R"({
            "udp_ip": "10.0.0.0",
            "udp_port": 12345,
            "http_port": 15,
            "session_timeout_sec": 80,
            "cdr_file": "file1.log",
            "graceful_shutdown_rate": 5,
            "log_file": "file2.log",
            "log_level": "DEBUG",
            "blacklist": ["012340123401234", "000111222333444"]
    })");
    ASSERT_TRUE(configFileExists());

    EXPECT_NO_THROW({
        // Act
        Config config(CONFIG_FILE);

        // Assert
        EXPECT_EQ(config.getUdpIp(), "10.0.0.0");
        EXPECT_EQ(config.getUdpPort(), 12345);
        EXPECT_EQ(config.getHttpPort(), 15);
        EXPECT_EQ(config.getSessionTimeoutSec(), 80);
        EXPECT_EQ(config.getCdrFile(), "file1.log");
        EXPECT_EQ(config.getGracefulShutdownRate(), 5);
        EXPECT_EQ(config.getLogFile(), "file2.log");
        EXPECT_EQ(config.getLogLevel(), "DEBUG");
        EXPECT_TRUE(config.getBlacklist().contains("012340123401234"));
        EXPECT_TRUE(config.getBlacklist().contains("000111222333444"));
    });
}

TEST_F(ConfigTest, UsesDefaultValues) {
    // Arrange
    createTestConfig(R"({
        // Поля отсутствуют
    })");

    EXPECT_NO_THROW({
        // Act
        Config config(CONFIG_FILE);
        // Assert
        EXPECT_EQ(config.getUdpIp(), pgw::constants::defaults::UDP_IP);
        EXPECT_EQ(config.getUdpPort(), pgw::constants::defaults::UDP_PORT);
        EXPECT_EQ(config.getHttpPort(), pgw::constants::defaults::HTTP_PORT);
        EXPECT_EQ(config.getSessionTimeoutSec(), pgw::constants::defaults::TIMEOUT_SEC);
        EXPECT_EQ(config.getCdrFile(), pgw::constants::defaults::CDR_FILE);
        EXPECT_EQ(config.getGracefulShutdownRate(), pgw::constants::defaults::GRACEFUL_SHUTDOWN_RATE);
        EXPECT_EQ(config.getLogFile(), pgw::constants::defaults::LOG_FILE);
        EXPECT_EQ(config.getLogLevel(), pgw::constants::defaults::LEVEL);
        EXPECT_EQ(config.getBlacklist().size(),0);
    });
}

TEST_F(ConfigTest, ThrowInvalidPort) {
    // Arrange
    createTestConfig(R"({
        "udp_port": 70000  // Невалидный порт
    })");

    EXPECT_NO_THROW({
        // Act
        Config config(CONFIG_FILE);

        // Assert
        EXPECT_FALSE(config.isValid());
        EXPECT_FALSE(config.getError().empty());
        EXPECT_EQ(config.getUdpPort(), pgw::constants::defaults::UDP_PORT);
    });
}