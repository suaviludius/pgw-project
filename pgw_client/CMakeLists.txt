add_executable(pgw_client
    main.cpp
)

target_sources(
    pgw_client PRIVATE
    Config.cpp
)

target_include_directories(
    pgw_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(
    pgw_client PRIVATE
    pgw_common
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)
