cmake_minimum_required(VERSION 3.15)
project(PGW_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CLIENT "Build client" ON)
option(BUILD_SERVER "Build server" ON)
option(BUILD_TESTS "Build tests" ON)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3  # Версия для стабильности
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# Общие исходники (статическая библиотека)
add_library(pgw_common STATIC
    src/logger.cpp
    src/Socket.cpp
)
target_include_directories(pgw_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(
    pgw_common PRIVATE
    spdlog::spdlog
)

if(BUILD_CLIENT)
    add_subdirectory(pgw_client)
endif()

if(BUILD_SERVER)
    add_subdirectory(pgw_server)
endif()

if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()

# Сообщение о конфигурации сборки
message(STATUS "=========================")
message(STATUS "  Build configuration:")
message(STATUS "  Server: ${BUILD_SERVER}")
message(STATUS "  Client: ${BUILD_CLIENT}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "=========================")