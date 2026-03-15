# Библиотека конфигурации клиента
add_library(pgw_client_config STATIC
    Config.cpp
)

target_include_directories(pgw_client_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(pgw_client_config PUBLIC
    pgw_common
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

add_executable(pgw_client
    main.cpp
)

# путь для этого конкретного файла
set_target_properties(pgw_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

target_link_libraries(pgw_client PRIVATE
    pgw_client_config
    httplib::httplib
)
