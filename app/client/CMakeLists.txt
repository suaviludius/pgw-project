add_executable(pgw_client
    main.cpp
)

target_sources(
    pgw_client PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/client/Config.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/client/Client.cpp
)

target_include_directories(pgw_client PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/client
)

target_link_libraries(
    pgw_client PRIVATE
    pgw_common
    httplib::httplib
)

configure_file(
    ${CMAKE_SOURCE_DIR}/configs/pgw_client.json
    COPYONLY
)