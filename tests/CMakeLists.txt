add_executable(test_config test_config.cpp)

# Добавляем файлы, которые будем тестировать
target_sources(test_config
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common/config.cpp
)

# Подключаем заголовочные файлы
target_include_directories(test_config
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
)


target_link_libraries(test_config
    PRIVATE
        GTest::gtest
        GTest::gtest_main   # Если не добавить, то Cmake будет пытаться создать main()
        nlohmann_json::nlohmann_json
)