#ifndef SESSION_H
#define SESSION_H

#include "types.h"
#include <chrono>

class Session {
public:
    using ClockType = std::chrono::steady_clock;
private:
    std::string_view m_imsi;
    ClockType::time_point m_createdTime;
public:
    Session(std::string_view imsi) : m_imsi{imsi}, m_createdTime{ClockType::now()}{
        // . . .
    };

    std::string_view getImsi(){ return m_imsi;}
    ClockType::time_point getCreatedTime(){ return m_createdTime;}

    std::chrono::seconds getAge(){
        return std::chrono::duration_cast<std::chrono::seconds>(ClockType::now() - m_createdTime);
    }
};

#endif // SESSION_H
