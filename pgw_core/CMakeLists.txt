# pgw_core - библиотека ядра PGW сервера
add_library(pgw_core STATIC
    src/UdpServer.cpp
    src/SessionManager.cpp
    src/CdrWriter.cpp
    src/Config.cpp
)

# Публичные заголовки (доступны при линковке)
target_include_directories(pgw_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/interfaces
)

# Зависимости
target_link_libraries(pgw_core PUBLIC
    pgw_common
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
