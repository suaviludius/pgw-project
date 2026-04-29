add_executable(pgw_server
    main.cpp
)

target_sources(
    pgw_server PRIVATE
    ../../src/pgw/server/Server.cpp
    ../../src/pgw/server/Config.cpp
    ../../src/pgw/server/HttpServer.cpp
    ../../src/pgw/server/FileCdrWriter.cpp
    ../../src/pgw/server/DatabaseCdrWriter.cpp
    ../../src/pgw/server/CdrWriterFactory.cpp
    ../../src/pgw/server/SessionManager.cpp
    ../../src/pgw/server/UdpServer.cpp
    ../../src/pgw/common/TcpSerializer.cpp
    ../../src/pgw/server/TcpHandler.cpp
    ../../src/pgw/server/TcpServer.cpp
)

set_target_properties(pgw_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_include_directories(
    pgw_server PRIVATE
    ../../include/pgw/server/interfaces
    ../../include/pgw/server
    ../../src/pgw/server
    ../../src/pgw/common
)

target_link_libraries(
    pgw_server PRIVATE
    pgw_common
    httplib::httplib
    SQLite::SQLite3
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../configs/pgw_server.json
    COPYONLY
)
