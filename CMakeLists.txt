cmake_minimum_required(VERSION 3.15)
project(PGW_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CLIENT "Build client" ON)
option(BUILD_SERVER "Build server" ON)
option(BUILD_TESTS "Build tests" ON)

# Кэширование зависимостей для ускорения
set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps CACHE PATH "Download directory for FetchContent")

include(FetchContent)

# Оптимизация FetchContent - отключаем обновления при каждом билде
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3  # Версия для стабильности
    GIT_SHALLOW TRUE # Загружаем только последний коммит! (Меньше времени на билд)
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(nlohmann_json spdlog httplib)

# Поиск системной SQLite3
find_package(SQLite3 REQUIRED)

# ========================================
# ========================================

# Общие исходники (статическая библиотека)
add_library(pgw_common STATIC)

target_sources(
    pgw_common PRIVATE
    src/pgw/common/logger.cpp
    src/pgw/common/UdpSocket.cpp
    src/pgw/common/TcpSocket.cpp
    src/pgw/common/SocketUtils.cpp
    src/pgw/common/SocketFactory.cpp
)

target_include_directories(pgw_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/pgw/common/interfaces # ... и то что будет видно другим
        ${CMAKE_CURRENT_SOURCE_DIR}/include/pgw/common # Интерфейсы ...
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pgw/common  # Для приватных заголовков (за что стыдно, то не видно)
)

# Линкуем зависимости
target_link_libraries(pgw_common
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    PRIVATE
        SQLite::SQLite3
)

# ========================================
# ========================================

if(BUILD_CLIENT)
    add_subdirectory(app/client)
endif()

if(BUILD_SERVER)
    add_subdirectory(app/server)
endif()

if(BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()

# ========================================
# ========================================

# Сообщение о конфигурации сборки
message(STATUS "=========================")
message(STATUS "  Build configuration:")
message(STATUS "  Server: ${BUILD_SERVER}")
message(STATUS "  Client: ${BUILD_CLIENT}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "=========================")