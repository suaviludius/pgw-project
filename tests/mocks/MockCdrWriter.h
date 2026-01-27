#ifndef MOCK_CDR_WRITER_H
#define MOCK_CDR_WRITER_H

#include "ICdrWriter.h"

#include <gmock/gmock.h>

class MockCdrWriter : public ICdrWriter {
public:
    // MockCdrWriter() = default;
    // virtual ~MockCdrWriter() = default;
    MOCK_METHOD(void, writeAction, (pgw::types::constImsi_t imsi, std::string_view action), (override));
};

#endif // MOCK_CDR_WRITER_H