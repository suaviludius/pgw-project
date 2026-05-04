#ifndef PGW_I_DATABASE_MANAGER_H
#define PGW_I_DATABASE_MANAGER_H

#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace pgw {

// CDR запись для возврата из БД
struct CdrRecord {
    std::string imsi;
    std::string action;
    std::string timestamp;
};

struct IDatabaseManager {
    // Лимит на чтение CDR записей из базы данных
    static constexpr size_t READ_CDR_LIMIT = 100;

    // Виртуальный диструктор - это бэйз, базиношвилли, базука
    virtual ~IDatabaseManager() = default;

    // Проверка подключения
    virtual bool isConnected() const = 0;

    // Запись CDR записи
    virtual bool writeCdr(std::string_view imsi, std::string_view action) = 0;

    // Получение последних CDR записей
    virtual std::vector<CdrRecord> getRecentCdr(size_t limit = READ_CDR_LIMIT) = 0;

    // Получение количества записей в таблице
    virtual std::optional<int> count(const std::string& table) = 0;
};

} // namespace pgw

#endif // PGW_I_DATABASE_MANAGER_H
