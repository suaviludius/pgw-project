#include "logger.h"
#include "pgw_client/Config.h"
#include "pgw_server/Config.h"
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
    static constexpr const char* CLIENT_CONFIG_FILE {"configs/pgw_client.json"};
    static constexpr const char* SERVER_CONFIG_FILE {"configs/pgw_server.json"};
    static constexpr const char* CDR_FILE {"test_integration_cdr.log"};
    static constexpr const char* LOG_FILE {"test_integration.log"};
    const pgw::types::seconds_t SESSION_TIMEOUT_S {3}; // Создаем свой таймаут для сессии, чтобы долго не ждать
    const pgw::types::imsi_t IMSI1 {"012340123401234"};
    const pgw::types::imsi_t IMSI2 {"000000000000001"};
    const pgw::types::imsi_t IMSI3 {"000000000000002"};
    const pgw::types::imsi_t IMSI4 {"000000000000003"};
    const pgw::types::imsi_t IMSI5 {"000000000000004"};
    const pgw::types::imsi_t IMSI6 {"000000000000005"};
    const pgw::types::imsi_t IMSI_BLACKLISTED {"101010101010101"};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {}
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        // Удаляем тестовые файлы
        std::filesystem::remove(CDR_FILE);
        std::filesystem::remove(LOG_FILE);
    }
};


TEST_F(IntegrationTest, FullUdpWork) {
    // Загружаем конфиг сервера
    pgw::server::Config configServer(SERVER_CONFIG_FILE);
    EXPECT_TRUE(configServer.isValid());

    // Инициализируем логгер с уровнем из конфига, но файлом из теста
    pgw::logger::init(
        LOG_FILE,
        configServer.getLogLevel()
    );
    EXPECT_TRUE(pgw::logger::isInit());

    LOG_INFO("SERVER ============= ");
    // Инициализируем компоненты
    CdrWriter cdrWriter(
        CDR_FILE
    );

    SessionManager sessionManager(cdrWriter,
        configServer.getBlacklist(),
        SESSION_TIMEOUT_S,
        configServer.getGracefulShutdownRate()
    );

    UdpServer udpServer(sessionManager,
        configServer.getUdpIp(),
        configServer.getUdpPort()
    );

    // Запускаем сервер
    udpServer.start();
    EXPECT_TRUE(udpServer.isRunning());

    // Даем время на обработку
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    LOG_INFO("CLIENT =============");
    // Загружаем конфиг сервера
    pgw::client::Config configClient(CLIENT_CONFIG_FILE);
    EXPECT_TRUE(configClient.isValid());

    // Создаем тестовый клиентский сокет
    Socket clientSocket;

    // Отправляем IMSI
    std::string imsi = IMSI1;
    LOG_INFO("Send imsi: {}", imsi);
    EXPECT_NO_THROW({
        clientSocket.send(imsi, configClient.getServerIp(), configClient.getServerPort());
    });

    // Обрабатываем прием пакета на сервере
    LOG_INFO("SERVER ============= ");
    udpServer.handler();

    // Проверяем что сессия создана и она одна
    EXPECT_TRUE(sessionManager.hasSession(imsi));
    EXPECT_EQ(sessionManager.countActiveSession(), 1);

    // Проверяем что запись в CDR произошла
    std::ifstream cdrFile(CDR_FILE);
    std::string cdrContent1((std::istreambuf_iterator<char>(cdrFile)),
                           std::istreambuf_iterator<char>());
    EXPECT_NE(cdrContent1.find(imsi), std::string::npos);
    EXPECT_NE(cdrContent1.find(ICdrWriter::SESSION_CREATED), std::string::npos);
    cdrFile.close();  // Закрываем файл для следующего чтения

    // Проверяем, что сессии не будут удаляться до истечения таймаута
    sessionManager.cleanTimeoutSessions();
    EXPECT_EQ(sessionManager.countActiveSession(), 1);

    LOG_INFO("Ожидаем таймаута сессии (~3c) и занесения записи в CDR");
    // Делаем задержку на время жизни сессии
    // PS: Для уменьшения задержки был введен свой параметр SESSION_TIMEOUT_S
    // если нужен прамаетр m_sessionTimeoutSec из configs/pgw_server.json
    // то используйте config.getSessionTimeout() и передавайте его в
    // инициализатор sessionManager
    std::this_thread::sleep_for(std::chrono::seconds(SESSION_TIMEOUT_S));

    // Проверяем, что сессии удаляются после таймаута
    sessionManager.cleanTimeoutSessions();
    EXPECT_EQ(sessionManager.countActiveSession(), 0);

    // Проверяем что запись в CDR произошла
    cdrFile.open(CDR_FILE);  // Открываем файл заново
    std::string cdrContent2((std::istreambuf_iterator<char>(cdrFile)),
                            std::istreambuf_iterator<char>());
    EXPECT_NE(cdrContent2.find(imsi), std::string::npos);
    EXPECT_NE(cdrContent2.find(ICdrWriter::SESSION_DELETED), std::string::npos);
    cdrFile.close();

    // Даем время на обработку
    std::this_thread::sleep_for(std::chrono::seconds());

    LOG_INFO("CLIENT =============");
    // Добавим в черный список новый IMSI
    imsi = IMSI_BLACKLISTED;
    sessionManager.addToBlacklist(imsi);
    LOG_INFO("Send blacklisted imsi: {}", imsi);
    EXPECT_NO_THROW({
        clientSocket.send(imsi, configClient.getServerIp(), configClient.getServerPort());
    });

    // Даем время на обработку
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Обрабатываем прием пакета на сервере
    LOG_INFO("SERVER ============= ");
    udpServer.handler();

    // Проверяем что запись в CDR произошла
    cdrFile.open(CDR_FILE);  // Открываем файл заново
    std::string cdrContent3((std::istreambuf_iterator<char>(cdrFile)),
                            std::istreambuf_iterator<char>());
    EXPECT_NE(cdrContent3.find(imsi), std::string::npos);
    EXPECT_NE(cdrContent3.find(ICdrWriter::SESSION_REJECTED), std::string::npos);
    cdrFile.close();

    LOG_INFO("CLIENT =============");
    EXPECT_NO_THROW({
        LOG_INFO("Send imsi: {}", IMSI2);
        clientSocket.send(IMSI2, configClient.getServerIp(), configClient.getServerPort());
        LOG_INFO("Sendend imsi: {}", IMSI3);
        clientSocket.send(IMSI3, configClient.getServerIp(), configClient.getServerPort());
        LOG_INFO("Send imsi: {}", IMSI4);
        clientSocket.send(IMSI4, configClient.getServerIp(), configClient.getServerPort());
        LOG_INFO("Send imsi: {}", IMSI5);
        clientSocket.send(IMSI5, configClient.getServerIp(), configClient.getServerPort());
    });

    // Даем время на обработку
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    LOG_INFO("SERVER ============= ");
    udpServer.handler();

    EXPECT_EQ(sessionManager.countActiveSession(), 4);

    LOG_INFO("SHUTDOWN ========== ");
    // Запускаем очищение сессий
    sessionManager.gracefulShutdown();

    EXPECT_EQ(sessionManager.countActiveSession(), 0);

    LOG_INFO("TEST DONE ========= ");
    // Останавливаем сервер
    udpServer.stop();
    EXPECT_FALSE(udpServer.isRunning());
}