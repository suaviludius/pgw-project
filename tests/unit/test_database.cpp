#include "DatabaseManager.h"
#include <gtest/gtest.h>
#include <filesystem>

struct DatabaseTest : public testing::Test {
    std::unique_ptr<pgw::DatabaseManager> db;
    static constexpr const char* DB_FILE {"test_pgw.db"};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}

    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        // Удаляем старую тестовую БД если есть
        std::filesystem::remove(DB_FILE);
        db = std::make_unique<pgw::DatabaseManager>(DB_FILE);
        ASSERT_TRUE(db->initialize());
    }

    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        db.reset();
        std::filesystem::remove(DB_FILE);
    }
};

TEST_F(DatabaseTest, Initialize_Success) {
    EXPECT_TRUE(db->isConnected());
}

TEST_F(DatabaseTest, WriteCdr_Success) {
    EXPECT_TRUE(db->writeCdr("001010123456789", "CREATED"));
    EXPECT_TRUE(db->writeCdr("001010123456790", "REJECTED"));

    auto records = db->getRecentCdr(10);
    EXPECT_EQ(records.size(), 2);
    EXPECT_EQ(records[0].imsi, "001010123456790");  // Последняя запись
    EXPECT_EQ(records[0].action, "REJECTED");
}

TEST_F(DatabaseTest, WriteLog_Success) {
    EXPECT_TRUE(db->writeLog("INFO", "Server started", "2026-03-27 20:20:26.312"));
    EXPECT_TRUE(db->writeLog("ERROR", "Connection failed", "2026-03-27 20:20:27.123"));

    auto events = db->getRecentEvents(10);
    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(events[0].level, "ERROR");
    EXPECT_EQ(events[0].message, "Connection failed");
    EXPECT_EQ(events[0].timestamp, "2026-03-27 20:20:27.123");
}

TEST_F(DatabaseTest, GetRecentCdr_Limit) {
    for (int i = 0; i < 50; ++i) {
        db->writeCdr("001010123456789", "CREATED");
    }

    auto records = db->getRecentCdr(10);
    EXPECT_EQ(records.size(), 10);
}

TEST_F(DatabaseTest, GetRecentEvents_Limit) {
    for (int i = 0; i < 50; ++i) {
        db->writeLog("INFO", "Event " + std::to_string(i), "2026-03-27 20:20:26.312");
    }

    auto events = db->getRecentEvents(20);
    EXPECT_EQ(events.size(), 20);
}

TEST_F(DatabaseTest, Count_Records) {
    db->writeCdr("001010123456789", "CREATED");
    db->writeCdr("001010123456790", "CREATED");
    db->writeCdr("001010123456791", "CREATED");

    auto count = db->count("cdr_records");
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(*count, 3);
}

TEST_F(DatabaseTest, CdrRecord_Fields) {
    db->writeCdr("001010123456789", "TERMINATED");

    auto records = db->getRecentCdr(1);
    ASSERT_EQ(records.size(), 1);

    // Проверяем что все поля заполнены
    EXPECT_FALSE(records[0].imsi.empty());
    EXPECT_FALSE(records[0].action.empty());
    EXPECT_FALSE(records[0].timestamp.empty());

    // Проверяем значения
    EXPECT_EQ(records[0].imsi, "001010123456789");
    EXPECT_EQ(records[0].action, "TERMINATED");
}

TEST_F(DatabaseTest, EventRecord_Fields) {
    db->writeLog("WARN", "High load detected", "2026-03-27 20:20:26.312");

    auto events = db->getRecentEvents(1);
    ASSERT_EQ(events.size(), 1);

    // Проверяем что все поля заполнены
    EXPECT_FALSE(events[0].level.empty());
    EXPECT_FALSE(events[0].message.empty());
    EXPECT_FALSE(events[0].timestamp.empty());

    // Проверяем значения
    EXPECT_EQ(events[0].level, "WARN");
    EXPECT_EQ(events[0].message, "High load detected");
    EXPECT_EQ(events[0].timestamp, "2026-03-27 20:20:26.312");
}
