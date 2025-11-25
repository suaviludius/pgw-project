// Общая конфигурация файла
//#include <nlohmann/json.cpp>
#include <string>   // Для string
#include <list>     // Для односвязного списка
#include <cstdint>  // Для uint_fast

class Config{

    std::string m_udpIp{};
    std::uint_fast16_t m_udpPort{};             // Максимальое число 9999, больше 2^8, но меньше 2^16
    std::uint_fast32_t m_sessionTimeoutSec{};   // Делаем больше, чтобы наверняка
    std::string m_cdrFile {};
    std::uint_fast16_t m_httpPort{};
    std::uint_fast32_t m_gracefulShutdownRate{};
    std::string m_logFile {};
    std::string m_logLevel {};
    std::list<std::string> m_blackList;       // Не требутеся доступ по индексу, поэтому можно использовать список

    std::string m_error{};                    // Ошибки при инициализации
    bool m_verification{true};                // Флаг отсутсвтия ошибок

public:
    explicit Config(const std::string& configPath); // Требуем вызов конструктора только с явным типом аргумента

    const std::string& getUdpIp() const { return m_udpIp; }
    uint16_t getUdpPort() const { return m_udpPort; }
    uint32_t getSessionTimeoutSec() const {return m_sessionTimeoutSec; }
    const std::string& getCdrFile() const { return m_cdrFile; }
    uint16_t getHttpPort() const { return m_httpPort; }
    uint32_t getGracefulShutdownRate() const { return m_gracefulShutdownRate; }
    const std::string& getLogFile() const { return m_logFile; }
    const std::string& getLogLevel() const { return m_logLevel; }
    const std::list<std::string>& getBlacklist() const { return m_blacklist; }
};