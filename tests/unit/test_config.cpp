#include "Config.h"
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
        std::filesystem::remove(CONFIG_FILE);
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // Всегда удаляем файл после теста
        std::filesystem::remove(CONFIG_FILE);
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
// + Неправильные типы данных
// + Граничные значения (порты 0, 65535 и т.д.)

TEST_F(ConfigTest, MissingFile){
    // Arrange
    // Act
    // Assert
    EXPECT_NO_THROW({
        pgw::server::Config config("non_existent_file.json");
    });
}

TEST_F(ConfigTest, SuccessfulReading) {
    // Arrange
    // R(raw string - спецсимволы не обрабатываются)
    createTestConfig( R"({
            "udp_ip": "0.0.0.0",
            "udp_port": 4567,
            "tcp_ip": "0.0.0.0",
            "tcp_port": 4321,
            "http_port": 1555,
            "session_timeout_sec": 80,
            "database_file": "file1.db",
            "cdr_file": "file2.log",
            "graceful_shutdown_rate": 5,
            "log_file": "file3.log",
            "log_level": "DEBUG",
            "blacklist": ["012340123401234", "000111222333444"]
    })");
    ASSERT_TRUE(configFileExists());

    EXPECT_NO_THROW({
        // Act
        pgw::server::Config config(CONFIG_FILE);

        std::cout << config.getError();

        // Assert
        EXPECT_EQ(config.getUdpIp(), "0.0.0.0");
        EXPECT_EQ(config.getUdpPort(), 4567);
        EXPECT_EQ(config.getTcpIp(), "0.0.0.0");
        EXPECT_EQ(config.getTcpPort(), 4321);
        EXPECT_EQ(config.getHttpPort(), 1555);
        EXPECT_EQ(config.getSessionTimeoutSec().count(), 80);
        EXPECT_EQ(config.getDatabaseFile(), "file1.db");
        EXPECT_EQ(config.getCdrFile(), "file2.log");
        EXPECT_EQ(config.getGracefulShutdownRate(), 5);
        EXPECT_EQ(config.getLogFile(), "file3.log");
        EXPECT_EQ(config.getLogLevel(), "DEBUG");
        EXPECT_NE(config.getBlacklist().find("012340123401234"), config.getBlacklist().end());
        EXPECT_NE(config.getBlacklist().find("000111222333444"), config.getBlacklist().end());
    });

}

TEST_F(ConfigTest, UsesDefaultValues) {
    // Arrange
    createTestConfig(R"({
        // Поля отсутствуют
    })");

    EXPECT_NO_THROW({
        // Act
        pgw::server::Config config(CONFIG_FILE);
        // Assert
        EXPECT_EQ(config.getUdpIp(), pgw::constants::server::defaults::UDP_IP);
        EXPECT_EQ(config.getUdpPort(), pgw::constants::server::defaults::UDP_PORT);
        EXPECT_EQ(config.getTcpIp(), pgw::constants::server::defaults::TCP_IP);
        EXPECT_EQ(config.getTcpPort(), pgw::constants::server::defaults::TCP_PORT);
        EXPECT_EQ(config.getHttpPort(), pgw::constants::server::defaults::HTTP_PORT);
        EXPECT_EQ(config.getSessionTimeoutSec().count(), pgw::constants::server::defaults::TIMEOUT_SEC);
        EXPECT_EQ(config.getDatabaseFile(), pgw::constants::server::defaults::DATABASE_FILE);
        EXPECT_EQ(config.getCdrFile(), pgw::constants::server::defaults::CDR_FILE);
        EXPECT_EQ(config.getGracefulShutdownRate(), pgw::constants::server::defaults::GRACEFUL_SHUTDOWN_RATE);
        EXPECT_EQ(config.getLogFile(), pgw::constants::server::defaults::LOG_FILE);
        EXPECT_EQ(config.getLogLevel(), pgw::constants::server::defaults::LEVEL);
        EXPECT_EQ(config.getBlacklist().size(),0);
    });
}

TEST_F(ConfigTest, BlacklistIsNotArray) {
    // Arrange
    createTestConfig(R"({
        "blacklist": "not array"
    })");

    EXPECT_NO_THROW({
        pgw::server::Config config(CONFIG_FILE);
        EXPECT_TRUE(config.isValid());
        EXPECT_EQ(config.getBlacklist().size(), 0);
    });
}

// Тесты валидаторов

TEST_F(ConfigTest, InvalidPort) {
    // Arrange
    createTestConfig(R"({
        "udp_port": 70000  // Невалидный порт
    })");

    EXPECT_NO_THROW({
        // Act
        pgw::server::Config config(CONFIG_FILE);

        // Assert
        EXPECT_FALSE(config.isValid());
        EXPECT_FALSE(config.getError().empty());
        EXPECT_EQ(config.getUdpPort(), pgw::constants::server::defaults::UDP_PORT);
    });
}

TEST_F(ConfigTest, InvalidImsiInBlacklist) {
    // Arrange
    createTestConfig(R"({
        "blacklist": [
            "123",
            "abc",
            "012340123401234"
        ]
    })");

    pgw::server::Config config(CONFIG_FILE);
    EXPECT_FALSE(config.isValid());
}

TEST_F(ConfigTest, InvalidSessionTimeout) {
    // Arrange
    createTestConfig(R"({
        "session_timeout_sec": 0
    })");

    pgw::server::Config config(CONFIG_FILE);
    EXPECT_FALSE(config.isValid());
}

