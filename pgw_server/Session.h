#ifndef PGW_SESSION_H
#define PGW_SESSION_H

#include "types.h"

// Класс, представляющий сессию абонента
// Хранит идентификатор абонента (IMSI) и время создания сессии
// Используется для отслеживания времени жизни сессии и автоматической
// очистки устаревших сессий по таймауту

struct Session {
    // Тип часов для измерения интервалов времени
    using clock = std::chrono::steady_clock;

    // IMSI (International Mobile Subscriber Identity) абонента
    pgw::types::imsi_t m_imsi;

    // Временная метка создания сессии
    clock::time_point m_createdTime;

    explicit Session(pgw::types::constImsi_t imsi)
    : m_imsi{imsi}, m_createdTime{clock::now()}{
    };
    ~Session() = default;

    // Вычисляет возраст сессии (время с момента создания)
    pgw::types::seconds_t getAge() const {
        auto age {std::chrono::duration_cast<std::chrono::seconds>(clock::now() - m_createdTime).count()};
        return static_cast<pgw::types::seconds_t>(age);
    }
};

#endif // PGW_SESSION_H
