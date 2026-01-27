add_executable(pgw_server
    main.cpp
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