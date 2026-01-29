#ifndef SESSION_H
#define SESSION_H

#include "types.h"

class Session {
public:
    // Супер! steady_clock - грамотный выбор часов
    using сlock = std::chrono::steady_clock; // Специальные часы для измерения интервалов
private:
    pgw::types::Imsi m_imsi;
    сlock::time_point m_createdTime; // Конкретная точка на временной шкале
public:
    explicit Session(pgw::types::ConstImsi imsi) : m_imsi{imsi}, m_createdTime{сlock::now()}{
        // . . .  /////////???????????????????? */
    };

    // Есть классная практика, для геттеров убираешь слово get и получается короче, а смысл не теряется
    // Потом в местах вызова session.imsi() ; session.age()
    pgw::types::ConstImsi getImsi() const { return m_imsi;}
    pgw::types::Seconds getAge() const {
        // ну и зачем переменную создавать? сразу всё это в return + вот тут и вылезает твой лишний каст в лишний тип Seconds
        auto age {std::chrono::duration_cast<std::chrono::seconds>(сlock::now() - m_createdTime).count()};
        return static_cast<pgw::types::Seconds>(age);
    }
};

#endif // SESSION_H
