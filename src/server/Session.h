#ifndef SESSION_H
#define SESSION_H

#include "types.h"

class Session {
public:
    using сlock = std::chrono::steady_clock; // Специальные часы для измерения интервалов
private:
    pgw::types::ConstImsi m_imsi;
    сlock::time_point m_createdTime; // Конкретная точка на временной шкале
public:
    explicit Session(pgw::types::ConstImsi imsi) : m_imsi{imsi}, m_createdTime{сlock::now()}{
        // . . .
    };

    pgw::types::ConstImsi getImsi() const { return m_imsi;}
    pgw::types::Seconds getAge() const {
        return std::chrono::duration_cast<pgw::types::Seconds>(сlock::now() - m_createdTime);
    }
};

#endif // SESSION_H
