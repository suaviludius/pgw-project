#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <list>
#include <chrono>

//  ATTENTION
//  В случае замены типа std::string на иной, необходимо также изменить
//  тип в файле constants.h на соответсвтующий.

namespace pgw::types {
    using Port = uint16_t;          // 0-65535 - достаточно uint16_t
    using Ip = std::string;
    using Seconds = int64_t;        // Тип для секунд
    using Rate = uint32_t;          // Делаем больше, чтобы наверняка
    using FilePath = std::string;
    using LogLevel = std::string;

    using ConstIp = std::string_view;
    using ConstFilePath = std::string_view;
    using ConstLogLevel = std::string_view;
    using ConstImsi = std::string_view;

    // Для черного списка сделаем свой тип, чтобы иметь удобный интерфейс
    class Blacklist {
    public:
        using list = std::list<std::string_view>;  // Не требутеся доступ по индексу, поэтому можно использовать список
    private:
        list m_blacklist;
    public:
        Blacklist() = default;
        explicit Blacklist(list blacklist) : m_blacklist{blacklist} {
            // . . .
        }

        bool contains(pgw::types::ConstImsi imsi) const {
            return std::find(m_blacklist.begin(), m_blacklist.end(), imsi) != m_blacklist.end();
        }

        void add(pgw::types::ConstImsi imsi) {
            m_blacklist.push_back(imsi);
        }

        // bool remove(pgw::types::ConstImsi imsi);

        // Итераторы
        auto begin() const { return m_blacklist.begin(); }
        auto end() const { return m_blacklist.end(); }
        // Размер
        size_t size() const { return m_blacklist.size(); }
        bool empty() const { return m_blacklist.empty(); }
        void clear() { m_blacklist.clear(); }
    };
}
#endif // TYPES_H