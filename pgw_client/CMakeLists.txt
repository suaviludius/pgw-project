add_executable(pgw_client
    main.cpp
)

# путь для этого конкретного файла
set_target_properties(pgw_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
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
