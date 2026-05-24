#ifndef PGW_I_DATABASE_MANAGER_H
#define PGW_I_DATABASE_MANAGER_H

#include "ICdrWriter.h"

#include <string>
#include <string_view>
#include <vector>

namespace pgw {


struct IDatabaseManager {
    // Виртуальный диструктор - это бэйз, базиношвилли, базука
    virtual ~IDatabaseManager() = default;

    // Проверка подключения
    virtual bool isConnected() const = 0;

    // Инициализация БД (без создания таблиц)
    virtual bool initialize() = 0;

    // Выполнение SQL без возврата результата
    virtual bool execute(const std::string& sql) = 0;
};

} // namespace pgw

#endif // PGW_I_DATABASE_MANAGER_H
