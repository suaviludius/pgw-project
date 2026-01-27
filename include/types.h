#ifndef PGW_TYPES_H
#define PGW_TYPES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <chrono>

//  ATTENTION
//  В случае замены типа std::string на иной, необходимо также изменить
//  тип в файле constants.h на соответсвтующий.

namespace pgw {
namespace types {

using port_t = uint16_t;          // 0-65535 - достаточно uint16_t
using ip_t = std::string;
using seconds_t = int64_t;        // Тип для секунд
using rate_t = int64_t;           // Тип для количества сессий (в секунду) на удаление
using filePath_t = std::string;
using logLevel_t = std::string;
using imsi_t = std::string;

using constIp_t = std::string_view;
using constFilePath_t = std::string_view;
using constLogLevel_t = std::string_view;
using constImsi_t = std::string_view;

// Для sessions и blacklist сделаем свой тип, чтобы иметь удобный интерфейс
template <typename T>
class Container {
public:
    using ctype = std::unordered_set<T>;  // Не требутеся доступ по индексу, поэтому можно использовать список
    using iterator = typename std::unordered_set<T>::iterator;
    using const_iterator = typename std::unordered_set<T>::const_iterator;
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
    iterator erase(iterator it) { return m_container.erase(it);}
    void clear() { m_container.clear();}
    size_t size() const { return m_container.size();}
    bool empty() const { return m_container.empty();}

    // Итераторы для поиска внутри контейнера
    iterator begin() { return m_container.begin(); }
    iterator end() { return m_container.end(); }
    // Константные версии
    const_iterator begin() const { return m_container.begin(); }  // const_iterator
    const_iterator end() const { return m_container.end(); }      // const_iterator
};

using Blacklist = pgw::types::Container<pgw::types::imsi_t>;


} // namespace types
} // namespace pgw

#endif // PGW_TYPES_H