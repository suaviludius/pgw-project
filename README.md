# Мини-PGW

Упрощенная модель сетевого компонента PGW (Packet Gateway)

## Функционал
- Event-driven архитектура на основе `poll()` без потоков для UDP
- Graceful shutdown с контролируемой скоростью удаления сессий
- HTTP API для мониторинга и управления
- CDR журналирование операций с сессиями
- Черный список IMSI для отклонения запросов
- Non-blocking UDP сервер без таймера на ожидание данных
- Конфигурация через JSON с валидацией
- Логирование с поддержкой уровней

## Требования
- C++17 компилятор (g++, clang 6+)
- CMake 3.15+
- Linux (тестировано на Ubuntu 20.04+)

## Сборка
```bash
git clone https://github.com/suaviludius/pgw-project.git
cd pgw-project
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Зависимости
Все зависимости загружаются автоматически через CMake `FetchContent`:
- nlohmann/json - парсинг JSON конфигурации
- cpp-httplib - HTTP сервер
- spdlog - логирование
- googletest - unit тесты (только для тестов)

## Конфигурация
Примеры файлов конфигурации лежат в папке config:
- Серверная конфигурация (`configs/pgw_server.json`)
- Клиентская конфигурация (`configs/pgw_client.json`)

## Использование
### Запуск сервера
```bash
# Запуск с конфигурацией по умолчанию
./build/src/pgw_server
# Запуск с указанием конфигурационного файла
./build/src/pgw_server /path/to/config.json
```
### Запуск клиента
```bash
# Отправка одного IMSI
./build/src/pgw_client 001010123456789
# Использование конфигурационного файла
./build/src/pgw_client config.json 001010123456789
```
### HTTP API
```bash
# Проверка статуса абонента
curl "http://localhost:8080/check_subscriber?imsi=001010123456789"
# Инициирование graceful shutdown
curl -X POST http://localhost:8080/stop
```
### Make команды
```bash
make            - Собрать проект (Release)
make configure  - Собрать конфигурацию CMake
make server     - Запустить сервер
make client     - Запустить клиент
make test       - Запустить тесты
make clean      - Удалить build директорию
make rebuild    - Полная пересборка
make help       - Показать эту справку
```

## UDP протокол

Формат запроса
- IMSI: 15 цифр в кодировке ASCII (Пример: "`001010123456789`")

Формат ответа
- "`created`" - сессия создана
- "`rejected`" - сессия отклонена (blacklist или уже существует)

## UML диаграммы
### Обработка UDP запроса
```mermaid
sequenceDiagram
    participant C as Client
    participant US as UdpServer
    participant SM as SessionManager
    participant CW as CdrWriter

    C->>US: 1: Запрос на создание сессии IMSI
    activate US
    US->>US: 2: Проверка правильности IMSI

    Note over C,CW: Результат проверки IMSI
    alt Неправильный
        US-->>C: 3: Ответ: cессия rejected
        deactivate US
    else Правильный: Created
        activate US
        US->>SM: 3: Создание сессии для IMSI
        activate SM
        SM->>SM: 4: Проверка IMSI по черному списку
        SM->>SM: 5: Проверка IMSI по активным сессиям
        SM->>CW: 6: Запись CDR: time IMSI "created"
        SM-->>US: 7: Результат создания сессии: CREATED
        deactivate SM
        US-->>C: 8: Ответ: cессия "created"
        deactivate US
    else Правильный: Rejected
        activate US
        US->>SM: 3: Создание сессии для IMSI
        activate SM
        SM->>SM: 4: Проверка IMSI по черному списку
        SM->>SM: 5: Проверка IMSI по активным сессиям
        SM->>CW: 6: Запись CDR: time IMSI "rejected"
        SM-->>US: 7: Результат создания сессии: REJECTED
        deactivate SM
        US-->>C: 8: Ответ: cессия "rejected"
        deactivate US
    end
```

### Структура классов cервера
```mermaid
classDiagram
    class ISessionManager {
        <<interface>>
        +CreateResult : enum class
        +hasSession(imsi : сonstImsi_t) bool
        +createSession(imsi : сonstImsi_t) CreateResult
        +countActiveSession() size_t
    }
    class SessionManager {
        -m_cdrWriter : ICdrWriter&
        -m_sessions : Container~unique_ptr~Session~~
        -m_blacklist : const Blacklist&
        -m_shutdownRate : const rate_t
        -m_sessionTimeoutSec : const seconds_t
        -findSession(imsi : constImsi_t) : iterator

        +SessionManager(...)
        +cleanTimeoutSessions() void
        +gracefulShutdown() void
        ...
    }
    class Session {
        -m_imsi : imsi_t
        -m_createdTime : time_point

        +Session(imsi : constImsi_t)
        +getImsi() constImsi_t
        +getAge() seconds_t
    }

    class ICdrWriter {
        <<interface>>
        +writeAction(imsi : сonstImsi_t, action : string_view) void
    }
    class CdrWriter {
        -m_file : ofstream

        +CdrWriter(filename : constFilePath_t)
    }

   class ISocket {
        <<interface>>
        +Packet : struct
        +bind(ip : ip_t, port : port_t) void
        +send(string data, sockaddr_in addr) void
        +receive() Packet
        +close() void
        +getFd() int
    }

    class Socket {
        -int m_fd

        +Socket()
    }

    class UdpServer {
        -m_sessionManager : ISessionManager&
        -m_socket : unique_ptr~ISocket~
        -m_ip : ip_t
        -m_port : port_t

        +UdpServer(...)
        +start() void
        +stop() void
        +handler() void
        +getFd() int
        +validateImsi(imsi : const std::string&)
    }

    class HttpServer {
        -Status : struct
        -m_sessionManager : ISessionManager&
        -m_server : unique_ptr~httplib::Server~
        -m_port : port_t
        -m_serverThread : thread
        -m_shutdownRequest : atomic~bool~&

        +HttpServer(...)
        +start() void
        +stop() void
        +setRoutes() void
        +handleSubscriberCheck(...) void
        +handleShutdown(...) void
    }

    UdpServer *-->"1" ISessionManager : использует
    UdpServer *-->"1" ISocket : содержит
    Socket -->"1" ISocket : реализует

    HttpServer *-->"1" ISessionManager : использует

    SessionManager -->"1" ISessionManager : реализует
    SessionManager *-->"1" ICdrWriter : использует
    SessionManager *-->"0..*" Session : содержит
    CdrWriter *-->"1" ICdrWriter : реализует
```

