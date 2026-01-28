#ifndef MOCK_SESSION_MANAGER_H
#define MOCK_SESSION_MANAGER_H

#include "ISessionManager.h"

#include <gmock/gmock.h>

class MockSessionManager : public ISessionManager {
public:
    // MockSessionManager() = default; // Конструктор по умолчанию - у интерфейса нет параметров
    // virtual ~MockSessionManager() = default;
    MOCK_METHOD(CreateResult, createSession, (pgw::types::constImsi_t imsi), (override));
    MOCK_METHOD(bool, hasSession, (pgw::types::constImsi_t imsi), (const, override));
    MOCK_METHOD(bool, addToBlacklist, (pgw::types::constImsi_t imsi), (override));
    MOCK_METHOD(size_t, countActiveSession, (), (const, override));
};

#endif // MOCK_SESSION_MANAGER_H