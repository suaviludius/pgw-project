# Исполнители тестов
add_executable(
    test_config
    test_config.cpp
)
add_executable(
    test_logger
    test_logger.cpp
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

# Подключаем заголовочные файлы
target_include_directories(
    test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
)
target_include_directories(
    test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
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