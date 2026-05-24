#ifndef MOCK_DATABASE_MANAGER_H
#define MOCK_DATABASE_MANAGER_H

#include "IDatabaseManager.h"

#include <gmock/gmock.h>

class MockDatabaseManager : public pgw::IDatabaseManager {
public:
    MOCK_METHOD(bool, isConnected, (), (const,override));
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(bool, execute, (const std::string& sql), (override));
};

#endif // MOCK_DATABASE_MANAGER_H
