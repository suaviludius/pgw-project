# Исполнители тестов
add_executable(
    test_config
    test_config.cpp
)
add_executable(
    test_logger
    test_logger.cpp
)
add_executable(
    test_session_manager
    test_session_manager.cpp
)

# Добавляем файлы, которые будем тестировать
target_sources(
    test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common/config.cpp
)
target_sources(
    test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common/logger.cpp
)
target_sources(
    test_session_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server/SessionManager.cpp
)

# Подключаем заголовочные файлы
target_include_directories(
    test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
)
target_include_directories(
    test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
)
target_include_directories(
    test_session_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/mocks # Важно указать перед common, где лежит настоящий Logger
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/server
)


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
    GTest::gtest
    GTest::gtest_main   # Если не добавить, то Cmake будет пытаться создать main()
    GTest::gmock        # Для mock
)