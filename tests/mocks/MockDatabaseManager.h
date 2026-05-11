#ifndef MOCK_DATABASE_MANAGER_H
#define MOCK_DATABASE_MANAGER_H

#include "IDatabaseManager.h"

#include <gmock/gmock.h>

class MockDatabaseManager : public pgw::IDatabaseManager {
public:
    MOCK_METHOD(bool, isConnected, (), (const,override));
    MOCK_METHOD(bool, writeCdr, (std::string_view imsi, std::string_view action), (override));
    MOCK_METHOD(std::vector<pgw::CdrRecord>, getRecentCdr, (size_t limit), (override));
    MOCK_METHOD( std::optional<int>, count, (const std::string& table), (override));
};

#endif // MOCK_DATABASE_MANAGER_H
