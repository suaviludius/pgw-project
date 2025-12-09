add_executable(pgw_server server/main.cpp)

target_sources(
    pgw_server PRIVATE
    common/config.cpp
    common/logger.cpp
    server/SessionManager.cpp
)

target_include_directories(
    pgw_server PRIVATE
    common
    server
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