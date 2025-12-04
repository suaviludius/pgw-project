#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <unordered_set>
#include <chrono>

//  ATTENTION
//  В случае замены типа std::string на иной, необходимо также изменить
//  тип в файле constants.h на соответсвтующий.

namespace pgw::types {
    using Port = uint16_t;          // 0-65535 - достаточно uint16_t
    using Ip = std::string;
    using Seconds = int64_t;        // Тип для секунд
    using Rate = int64_t;           // Тип для миллисекунд
    using FilePath = std::string;
    using LogLevel = std::string;

    using ConstIp = std::string_view;
    using ConstFilePath = std::string_view;
    using ConstLogLevel = std::string_view;
    using ConstImsi = std::string_view;

    // Для sessions и blacklist сделаем свой тип, чтобы иметь удобный интерфейс
    template <typename T>
    class Container {
    public:
        using ctype = std::unordered_set<T>;  // Не требутеся доступ по индексу, поэтому можно использовать список
    private:
        ctype m_container;
    public:
        Container() = default;
        explicit Container(ctype container) : m_container(std::move(container)) {
            // . . .
        }

        bool add(T value) {
            auto [it, inserted] = m_container.insert(std::move(value)); // it - итератор на элемент, inserted - успех операции
            return inserted;
        }

        bool contains(const T& value) const { return m_container.find(value) != m_container.end();}
        std::iterator erase(std::iterator it) { return m_container.erase(it);}
        void clear() { m_container.clear();}
        size_t size() const { return m_container.size();}
        bool empty() const { return m_container.empty();}

        // Итераторы для поиска внутри контейнера
        auto begin() const { return m_set.begin(); }
        auto end() const { return m_set.end(); }
    };
}
#endif // TYPES_H