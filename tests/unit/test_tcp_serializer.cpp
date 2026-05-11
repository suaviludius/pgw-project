#include "TcpSerializer.h"

#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>

// Фикстура для всех тестов
struct TcpSerializerTest : public testing::Test {

    // Тестовое сообщение
    pgw::protocol::Message msg;

    // Метод, вызываемый перед всеми тестами
    static void SetUpTestSuit(){}
    // Метод, вызываемый после всех тестов
    static void TearDowmTestSuite(){}
    // Метод, вызываемый в начале каждого теста
    void SetUp() override {
        msg.header.version = pgw::protocol::PROTOCOL_VERSION;
        msg.header.command = pgw::protocol::Command::TEST;
        msg.header.status = pgw::protocol::Status::TEST;
        msg.header.length = 0;
    }
    // Метод, вызываемый в конце каждого теста
    void TearDown() override {
    }
};

// Проврка сообзения без данных, только заголовок
TEST_F(TcpSerializerTest, SerializeAndDeserializeMessageHeader){
    // Arrange
    // Act
    auto buffer = pgw::TcpSerializer::serializer(msg);
    auto resultMsg = pgw::TcpSerializer::deserializer(buffer);
    // Assert
    ASSERT_TRUE(resultMsg.has_value());
    EXPECT_EQ(resultMsg->header.command, msg.header.command);
    EXPECT_EQ(resultMsg->header.status, msg.header.status);
}

TEST_F(TcpSerializerTest, SerializeAndDeserializeMessageJson){
    // Arrange
    std::string imsi = "1234567890";
    nlohmann::json jsonData = {{"imsi", imsi}};
    // Act
    auto msgWithData = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::TEST,
        pgw::protocol::Status::TEST,
        jsonData
    );
    auto buffer = pgw::TcpSerializer::serializer(msgWithData);
    auto resultMsg = pgw::TcpSerializer::deserializer(buffer);
    // Assert
    ASSERT_TRUE(resultMsg.has_value());
    auto jsonDataFromMsg = pgw::TcpSerializer::getJsonData(*resultMsg);
    EXPECT_EQ(jsonDataFromMsg["imsi"], imsi);
}

TEST_F(TcpSerializerTest, DeserializeWrongVersion) {
    auto buffer = pgw::TcpSerializer::serializer(msg);
    buffer[0] = 0xFF; // Портим версию
    auto resultMsg = pgw::TcpSerializer::deserializer(buffer);
    EXPECT_FALSE(resultMsg.has_value());
}

TEST_F(TcpSerializerTest, DeserializeIncompleteData) {
    // Arrange
    // Act
    auto buffer = pgw::TcpSerializer::serializer(msg);
    // Удаляем последний байт заголовка
    buffer.pop_back();
    auto resultMsg = pgw::TcpSerializer::deserializer(buffer);
    // Assert
    // Должен вернуть nullopt
    EXPECT_FALSE(resultMsg.has_value());
}

TEST_F(TcpSerializerTest, CreateAndGetJsonMessage) {
    // Arrange
    std::string imsi = "1234567890";
    std::string action = "test";
    nlohmann::json jsonData = {{"action", action}, {"imsi", imsi}};
    // Act
    auto msgWithData = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::TEST,
        pgw::protocol::Status::TEST,
        jsonData
    );
    auto actualJsonData = pgw::TcpSerializer::getJsonData(msgWithData);
    // Assert
    EXPECT_EQ(actualJsonData["action"], action);
    EXPECT_EQ(actualJsonData["imsi"], imsi);
}

TEST_F(TcpSerializerTest, GetJsonFromEmptyMessage) {
    // Arrange
    pgw::protocol::Message emptyMsg;
    emptyMsg.header.length = 0;
    emptyMsg.data.clear();
    // Act
    auto json = pgw::TcpSerializer::getJsonData(emptyMsg);
    // Assert
    EXPECT_TRUE(json.is_object() && json.empty());
}

TEST_F(TcpSerializerTest, FullCommandRoundtrip) {
    // Arrange
    std::string imsi = "1234567890";
    ssize_t limit = 10;
    nlohmann::json jsonData = {{"imsi", imsi}, {"limit", limit}};
    // Act
    auto msgWithData = pgw::TcpSerializer::createJsonMsg(
        pgw::protocol::Command::GET_CDR,
        pgw::protocol::Status::OK,
        jsonData
    );
    auto buffer = pgw::TcpSerializer::serializer(msgWithData);
    auto resultMsg = pgw::TcpSerializer::deserializer(buffer);
    ASSERT_TRUE(resultMsg.has_value());
    // Assert
    EXPECT_EQ(resultMsg->header.command, pgw::protocol::Command::GET_CDR);
    auto params = pgw::TcpSerializer::getJsonData(*resultMsg);
    EXPECT_EQ(params["imsi"], imsi);
    EXPECT_EQ(params["limit"], limit);
}
