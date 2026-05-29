#include "DatabaseCdrWriter.h"
#include "DatabaseManager.h"
#include "pgw/common/types.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <memory>

struct DatabaseCdrWriterTest : public testing::Test {
    static constexpr const char* DB_FILE = "test_cdr.db";
    const pgw::types::imsi_t IMSI1 = "012340123401234";
    const pgw::types::imsi_t IMSI2 = "000111222333444";
    const pgw::types::imsi_t IMSI3 = "111111111111111";

    std::shared_ptr<pgw::DatabaseManager> dbManager;
    std::unique_ptr<pgw::DatabaseCdrWriter> cdrWriter;

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}

    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        std::filesystem::remove(DB_FILE);
        dbManager = std::make_shared<pgw::DatabaseManager>(DB_FILE);
        ASSERT_TRUE(dbManager->initialize());
        cdrWriter = std::make_unique<pgw::DatabaseCdrWriter>(dbManager);
    }

    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
        cdrWriter.reset();
        dbManager.reset();
        std::filesystem::remove(DB_FILE);
    }
};

TEST_F(DatabaseCdrWriterTest, WriteAndReadRecords) {
    cdrWriter->writeAction(IMSI1, pgw::ICdrWriter::SESSION_CREATED);
    cdrWriter->writeAction(IMSI2, pgw::ICdrWriter::SESSION_REJECTED);

    cdrWriter->flush();

    auto records = cdrWriter->getRecentRecords(10);
    ASSERT_EQ(records.size(), 2);
    EXPECT_EQ(records[0].imsi, IMSI2);   // последняя запись
    EXPECT_EQ(records[0].action, pgw::ICdrWriter::SESSION_REJECTED);
    EXPECT_EQ(records[1].imsi, IMSI1);
}

TEST_F(DatabaseCdrWriterTest, GetRecentRecordsLimit) {
    const size_t cdrCount = pgw::ICdrWriter::READ_CDR_LIMIT + 10;
    for (size_t i = 0; i < cdrCount; ++i) {
        cdrWriter->writeAction(IMSI1, pgw::ICdrWriter::SESSION_CREATED);
    }

    cdrWriter->flush();

    auto records = cdrWriter->getRecentRecords(cdrCount);
    EXPECT_LE(records.size(), pgw::ICdrWriter::READ_CDR_LIMIT);
}

TEST_F(DatabaseCdrWriterTest, RecordCount) {
    cdrWriter->writeAction(IMSI1, pgw::ICdrWriter::SESSION_CREATED);
    cdrWriter->writeAction(IMSI2, pgw::ICdrWriter::SESSION_CREATED);
    cdrWriter->writeAction(IMSI3, pgw::ICdrWriter::SESSION_CREATED);

    cdrWriter->flush();

    auto count = cdrWriter->getRecordCount();
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(*count, 3);
}

TEST_F(DatabaseCdrWriterTest, RecordFields) {
    cdrWriter->writeAction(IMSI1, pgw::ICdrWriter::SESSION_ERROR);

    cdrWriter->flush();

    auto records = cdrWriter->getRecentRecords(1);
    ASSERT_EQ(records.size(), 1);

    // Проверяем что все поля заполнены
    EXPECT_FALSE(records[0].imsi.empty());
    EXPECT_FALSE(records[0].action.empty());
    EXPECT_FALSE(records[0].timestamp.empty());

    // Проверяем значения
    EXPECT_EQ(records[0].imsi, IMSI1);
    EXPECT_EQ(records[0].action, pgw::ICdrWriter::SESSION_ERROR);
}