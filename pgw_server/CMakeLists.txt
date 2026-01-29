add_executable(pgw_server
    main.cpp
)

# исправляем то, что бинари не в папке bin
set_target_properties(pgw_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

target_sources(
    pgw_server PRIVATE
    Config.cpp
    HttpServer.cpp
    CdrWriter.cpp
    SessionManager.cpp
    UdpServer.cpp
)

target_include_directories(
    pgw_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
)

target_link_libraries(
    pgw_server PRIVATE
    # Библиотека с logger и Socket
    pgw_common
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../configs/pgw_server.json
    COPYONLY
)
