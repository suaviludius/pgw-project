#include "logger.h"
#include "Config.h"
#include "CdrWriter.h"
#include "SessionManager.h"
#include "Socket.h"
#include "UdpServer.h"
#include "HttpServer.h"

#include <gtest/gtest.h>

#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

struct IntegrationTest : public testing::Test {
    static constexpr const char* CONFIG_FILE {"configs/pgw_server.json"};
    static constexpr const char* CDR_FILE {"test_integration_cdr.log"};
    static constexpr const char* LOG_FILE {"test_integration.log"};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {}
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {}

    void TearDown() override {
        // Удаляем тестовые файлы
        std::filesystem::remove(CONFIG_FILE);
        std::filesystem::remove(CDR_FILE);
        std::filesystem::remove(LOG_FILE);
    }
};

TEST_F(IntegrationTest, FullUdpWork) {
    // Загружаем конфиг
    pgw::server::Config config(CONFIG_FILE);
    EXPECT_TRUE(config.isValid());

    // Инициализируем логгер параметрами из конфиг файла
    logger::init(
        LOG_FILE,
        config.getLogLevel()
    );
    EXPECT_TRUE(logger.isInit());

    // Инициализируем компоненты
    CdrWriter cdrWriter(
        CDR_FILE
    );

    SessionManager sessionManager(cdrWriter,
        config.getBlacklist(),
        config.getSessionTimeoutSec(),
        config.getGracefulShutdownRate()
    );

    UdpServer udpServer(sessionManager,
        config.getUdpIp(),
        config.getUdpPort()
    );

    // Запускаем сервер
    udpServer.start();
    EXPECT_TRUE(udpServer.isRunning());

    // Создаем тестовый клиентский сокет
    Socket clientSocket;

    // Отправляем IMSI
    std::string imsi = "123456789012345";
    EXPECT_NO_THROW({
        clientSocket.send(imsi, config.getUdpIp(), config.getUdpPort());
    });

    // Даем время на обработку
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Проверяем что сессия создана
    EXPECT_TRUE(sessionManager.hasSession(imsi));

    // Проверяем что запись в CDR произошла
    std::ifstream cdrFile(CDR_FILE);
    std::string cdrContent((std::istreambuf_iterator<char>(cdrFile)),
                           std::istreambuf_iterator<char>());
    EXPECT_NE(cdrContent.find(imsi), std::string::npos);
    EXPECT_NE(cdrContent.find("created"), std::string::npos);

    // Останавливаем сервер
    udpServer.stop();
    EXPECT_FALSE(udpServer.isRunning());
}