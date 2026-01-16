add_executable(pgw_server
    server/main.cpp
)

target_sources(
    pgw_server PRIVATE
    common/config.cpp
    common/logger.cpp
    server/HttpServer.cpp
    server/Socket.cpp
    server/CdrWriter.cpp
    server/SessionManager.cpp
    server/UdpServer.cpp
)

target_include_directories(
    pgw_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/server/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/server
)

target_link_libraries(
    pgw_server PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)

add_executable(pgw_client client/main.cpp)

target_link_libraries(
    pgw_client PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)