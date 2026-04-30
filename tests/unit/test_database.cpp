#include "DatabaseManager.h"
#include "types.h"

#include <gtest/gtest.h>
#include <filesystem>

struct DatabaseTest : public testing::Test {
    std::unique_ptr<pgw::DatabaseManager> db;
    static constexpr const char* DB_FILE {"test_pgw.db"};
    const pgw::types::imsi_t IMSI1 {"012340123401234"};
    const pgw::types::imsi_t IMSI2 {"000111222333444"};
    const pgw::types::imsi_t IMSI3 {"111111111111111"};

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

TEST_F(DatabaseTest, InitializeSuccess) {
    EXPECT_TRUE(db->isConnected());
}

TEST_F(DatabaseTest, InitializeInvalidPath) {
    pgw::DatabaseManager dbIP("/invalid/path/test.db");
    EXPECT_FALSE(dbIP.initialize());
    EXPECT_FALSE(dbIP.isConnected());
}

TEST_F(DatabaseTest, WriteCdrSuccess) {
    EXPECT_TRUE(db->writeCdr(IMSI1, "CREATED"));
    EXPECT_TRUE(db->writeCdr(IMSI2, "REJECTED"));

    auto records = db->getRecentCdr(10);
    EXPECT_EQ(records.size(), 2);
    EXPECT_EQ(records[0].imsi, IMSI2);  // Последняя запись
    EXPECT_EQ(records[0].action, "REJECTED");
}

TEST_F(DatabaseTest, GetRecentCdrLimit) {
    const size_t cdrCount = pgw::DatabaseManager::READ_CDR_LIMIT + 10;
    for (int i = 0; i < cdrCount; ++i) {
        db->writeCdr(IMSI1, "CREATED");
    }

    auto records = db->getRecentCdr(cdrCount);
    EXPECT_LE(records.size(), pgw::DatabaseManager::READ_CDR_LIMIT);
}


TEST_F(DatabaseTest, CountRecords) {
    db->writeCdr(IMSI1, "CREATED");
    db->writeCdr(IMSI2, "CREATED");
    db->writeCdr(IMSI3, "CREATED");

    auto count = db->count("cdr_records");
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(*count, 3);
}

TEST_F(DatabaseTest, CdrRecordFields) {
    db->writeCdr(IMSI1, "TERMINATED");

    auto records = db->getRecentCdr(1);
    ASSERT_EQ(records.size(), 1);

    // Проверяем что все поля заполнены
    EXPECT_FALSE(records[0].imsi.empty());
    EXPECT_FALSE(records[0].action.empty());
    EXPECT_FALSE(records[0].timestamp.empty());

    // Проверяем значения
    EXPECT_EQ(records[0].imsi, IMSI1);
    EXPECT_EQ(records[0].action, "TERMINATED");
}
