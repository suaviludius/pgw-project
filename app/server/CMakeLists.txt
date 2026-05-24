add_executable(pgw_server
    main.cpp
)

target_sources(
    pgw_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/server/Server.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/Config.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/HttpServer.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/FileCdrWriter.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/DatabaseCdrWriter.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/CdrWriterFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/SessionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/UdpServer.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/TcpHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/TcpServer.cpp
)

target_include_directories(
    pgw_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include/pgw/server
    ${CMAKE_SOURCE_DIR}/include/pgw/server/interfaces
    ${CMAKE_SOURCE_DIR}/src/pgw/server
)

target_link_libraries(
    pgw_server PRIVATE
    pgw_common
    httplib::httplib
    SQLite::SQLite3
)

configure_file(
    ${CMAKE_SOURCE_DIR}/configs/pgw_server.json
    COPYONLY
)
