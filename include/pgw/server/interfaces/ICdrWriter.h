#ifndef I_CDR_WRITER
#define I_CDR_WRITER

#include "pgw/common/types.h"

#include <optional>
#include <vector>

namespace pgw {

// CDR запись для возврата из БД
struct CdrRecord {
    std::string imsi;
    std::string action;
    std::string timestamp;
};

struct ICdrWriter {
    // Литералы для записи сообщений CDR
    static constexpr std::string_view SESSION_CREATED = "CREATED";
    static constexpr std::string_view SESSION_DELETED = "DELETED";
    static constexpr std::string_view SESSION_REJECTED = "REJECTED";
    static constexpr std::string_view SESSION_ERROR = "ERROR";

    // Лимит на чтение CDR записей из источника
    static constexpr size_t READ_CDR_LIMIT = 100;

    virtual ~ICdrWriter() = default;

    // Запись события
    // Формат записи: IMSI, ACTION, TIMESTAMP
    virtual void writeAction(pgw::types::constImsi_t imsi, std::string_view action) = 0;

    // Чтение события из хранилища CDR
    virtual std::vector<CdrRecord> getRecentRecords(size_t limit = 100) = 0;

    // Количество записей сделано
    virtual std::optional<int> getRecordCount() = 0;
};

} // namespace pgw

#endif // I_CDR_WRITER
