# Исполнители тестов
add_executable(
    test_config
    unit/test_config.cpp
)
add_executable(
    test_logger
    unit/test_logger.cpp
)
add_executable(
    test_session_manager
    unit/test_session_manager.cpp
)
add_executable(
    test_udp_server
    unit/test_udp_server.cpp
)
add_executable(
    test_database
    unit/test_database.cpp
)
add_executable(
    test_integration
    integration/test_integration.cpp
)

# Добавляем файлы, которые будем тестировать
target_sources(
    test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/server/Config.cpp
)
target_sources(
    test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/common/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/common/DatabaseManager.cpp
)
target_sources(
    test_session_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/server/SessionManager.cpp
)
target_sources(
    test_udp_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/server/UdpServer.cpp
)
target_sources(
    test_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/client/Config.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/Config.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/FileCdrWriter.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/DatabaseCdrWriter.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/CdrWriterFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/SessionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/pgw/server/UdpServer.cpp
)

# Подключаем заголовочные файлы
target_include_directories(
    test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/server
)
target_include_directories(
    test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/pgw/server
)
target_include_directories(
    test_session_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/include/pgw/server/interfaces
    ${CMAKE_SOURCE_DIR}/include/pgw/server
    ${CMAKE_SOURCE_DIR}/src/pgw/server
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)
target_include_directories(
    test_udp_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include/pgw/server/interfaces
    ${CMAKE_SOURCE_DIR}/include/pgw/server
    ${CMAKE_SOURCE_DIR}/src/pgw/server
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)
target_include_directories(
    test_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include/pgw/server/interfaces
    ${CMAKE_SOURCE_DIR}/include/pgw/server
    ${CMAKE_SOURCE_DIR}/include/pgw/client
    ${CMAKE_SOURCE_DIR}/src/pgw/server
    ${CMAKE_SOURCE_DIR}/src/pgw/client
)

# Зависимости загруженны в главном CMakeList
target_link_libraries(
    test_config PRIVATE
    GTest::gtest
    GTest::gtest_main   # Если не добавить, то Cmake будет пытаться создать main()
    nlohmann_json::nlohmann_json
)
target_link_libraries(
    test_logger PRIVATE
    pgw_common
    GTest::gtest
    GTest::gtest_main   # Если не добавить, то Cmake будет пытаться создать main()
    spdlog::spdlog
)
target_link_libraries(
    test_session_manager PRIVATE
    pgw_common
    GTest::gtest
    GTest::gtest_main   # Если не добавить, то Cmake будет пытаться создать main()
    GTest::gmock        # Для mock
)
target_link_libraries(
    test_udp_server PRIVATE
    pgw_common
    GTest::gtest
    GTest::gtest_main   # Если не добавить, то Cmake будет пытаться создать main()
    GTest::gmock        # Для mock
)
target_link_libraries(
    test_database PRIVATE
    pgw_common
    GTest::gtest
    GTest::gtest_main
    SQLite::SQLite3
)
target_link_libraries(
    test_integration PRIVATE
    pgw_common
    GTest::gtest
    GTest::gtest_main   # Если не добавить, то Cmake будет пытаться создать main()
    GTest::gmock        # Для mock
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)

# Копируем конфиги в build директорию для интеграционных тестов
configure_file(
    ${CMAKE_SOURCE_DIR}/configs/pgw_server.json
    ${CMAKE_BINARY_DIR}/configs/pgw_server.json
    COPYONLY
)

# Регистрация тестов в CTest
enable_testing()
add_test(NAME test_config COMMAND test_config)
add_test(NAME test_logger COMMAND test_logger)
add_test(NAME test_session_manager COMMAND test_session_manager)
add_test(NAME test_udp_server COMMAND test_udp_server)
add_test(NAME test_database COMMAND test_database)
add_test(NAME test_integration COMMAND test_integration WORKING_DIRECTORY ${CMAKE_BINARY_DIR})