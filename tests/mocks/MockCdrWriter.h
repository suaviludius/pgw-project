#ifndef MOCK_CDR_WRITER_H
#define MOCK_CDR_WRITER_H

#include "ICdrWriter.h"

#include <gmock/gmock.h>

class MockCdrWriter : public pgw::ICdrWriter {
public:
    // MockCdrWriter() = default;
    // virtual ~MockCdrWriter() = default;
    MOCK_METHOD(void, writeAction, (pgw::types::constImsi_t imsi, std::string_view action), (override));
    MOCK_METHOD(std::vector<pgw::CdrRecord>, getRecentRecords, (size_t limit), (override));
    MOCK_METHOD(std::optional<int>, getRecordCount, (), (override));
};

#endif // MOCK_CDR_WRITER_H

