add_executable(pgw_client
    main.cpp
)

target_sources(
    pgw_client PRIVATE
    ../../src/pgw/client/Config.cpp
    ../../src/pgw/client/Client.cpp
)

# путь для этого конкретного файла
set_target_properties(pgw_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)


# TODO: нужно сделать так, чтобы pgw_common файлы собирались и оставляли интерфейс
# для остальных дирректорий, чтобы те могли им пользоваться, а не подключать целую дирректорию
# ради пары файлов. Смысл тогда было рефакторить проект, если статическая библиотека собирается
# и в приложении и до него!
target_include_directories(pgw_client PRIVATE
    ../../src/pgw/common
    ../../include/pgw/client
    ../../src/pgw/client
)

target_link_libraries(
    pgw_client PRIVATE
    pgw_common
    httplib::httplib
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../configs/pgw_client.json
    COPYONLY
)