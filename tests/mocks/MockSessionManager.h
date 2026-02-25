#ifndef MOCK_SESSION_MANAGER_H
#define MOCK_SESSION_MANAGER_H

#include "ISessionManager.h"

#include <gmock/gmock.h>

class MockSessionManager : public ISessionManager {
public:
    // MockSessionManager() = default; // Конструктор по умолчанию - у интерфейса нет параметров
    // virtual ~MockSessionManager() = default;
    MOCK_METHOD(CreateResult, createSession, (const pgw::types::imsi_t& imsi), (override));
    MOCK_METHOD(bool, hasSession, (const pgw::types::imsi_t& imsi), (const, override));
    MOCK_METHOD(bool, addToBlacklist, (const pgw::types::imsi_t& imsi), (override));
    MOCK_METHOD(size_t, countActiveSession, (), (const, override));
};

#endif // MOCK_SESSION_MANAGER_H