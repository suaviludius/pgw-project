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
    test_integration
    integration/test_integration.cpp
)

# Добавляем файлы, которые будем тестировать
target_sources(
    test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/pgw_core/src/Config.cpp
)
target_sources(
    test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/logger.cpp
)

# test_session_manager, test_udp_server, test_integration теперь используют pgw_core

# Подключаем заголовочные файлы
target_include_directories(
    test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/pgw_core/include
)
target_include_directories(
    test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_include_directories(
    test_session_manager PRIVATE
    # ОБЯЗАТЕЛЬНО! mocks подключать перед include
    # в нём содержится тестовая версия логера
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_SOURCE_DIR}/include
)
target_include_directories(
    test_udp_server PRIVATE
    # ОБЯЗАТЕЛЬНО! mocks подключать перед include
    # в нём содержится тестовая версия логера
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_SOURCE_DIR}/include
)
target_include_directories(
    test_integration PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/pgw_client
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
    GTest::gtest
    GTest::gtest_main   # Если не добавить, то Cmake будет пытаться создать main()
    spdlog::spdlog
)
target_link_libraries(
    test_session_manager PRIVATE
    pgw_core
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
target_link_libraries(
    test_udp_server PRIVATE
    pgw_core
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
target_link_libraries(
    test_integration PRIVATE
    pgw_core
    pgw_client_config
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)