#include "DatabaseManager.h"
#include "types.h"

#include <gtest/gtest.h>
#include <filesystem>

struct DatabaseManagerTest : public testing::Test {
    static constexpr const char* DB_FILE {"test_pgw.db"};

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}

    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        // Удаляем старую тестовую БД если есть
        std::filesystem::remove(DB_FILE);
    }

    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        std::filesystem::remove(DB_FILE);
    }
};

TEST_F(DatabaseManagerTest, InitializeSuccess) {
    pgw::DatabaseManager db(DB_FILE);
    EXPECT_TRUE(db.initialize());
    EXPECT_TRUE(db.isConnected());
}

TEST_F(DatabaseManagerTest, InitializeInvalidPath) {
    pgw::DatabaseManager dbIP("/invalid/path/test.db");
    EXPECT_FALSE(dbIP.initialize());
    EXPECT_FALSE(dbIP.isConnected());
}

TEST_F(DatabaseManagerTest, ExecuteSql) {
    pgw::DatabaseManager db(DB_FILE);
    ASSERT_TRUE(db.initialize());
    EXPECT_TRUE(db.execute("CREATE TABLE test (id INTEGER);"));
    EXPECT_TRUE(db.execute("INSERT INTO test VALUES (42);"));
}

TEST_F(DatabaseManagerTest, PrepareStatement) {
    pgw::DatabaseManager db(DB_FILE);
    ASSERT_TRUE(db.initialize());
    ASSERT_TRUE(db.execute("CREATE TABLE test (val INTEGER);"));

    auto stmt = db.prepareStatement("SELECT * FROM test");
    EXPECT_NE(stmt, nullptr);
    sqlite3_finalize(stmt);
}
