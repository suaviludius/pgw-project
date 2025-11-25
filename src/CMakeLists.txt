add_executable(pgw_server server/main.cpp)

target_link_libraries(pgw_server
    PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        httplib::httplib
)

add_executable(pgw_client client/main.cpp)

target_link_libraries(pgw_client
    PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)