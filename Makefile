# MakeFile for CMake project

# Конфигурации
BUILD_DIR := build
CONFIG := Release

# Имя исполняемого файла
TARGET_EXEC1 := pgw_server
TARGET_EXEC2 := pgw_client

# Все доступные команды(цели) для make
.PHONY: all configure build run clean help

# По умолчанию простая сборка
all:		configure build

# Конфигурация Cmake
# Если не удалять закешированные данные, то выставленные опции не будут применяться
configure:
			@rm -rf $(BUILD_DIR)/CMakeCache.txt $(BUILD_DIR)/CMakeFiles
			@mkdir -p $(BUILD_DIR)
			@cd $(BUILD_DIR) && cmake ..

#Сборка проекта
build:
			@cd $(BUILD_DIR) && cmake --build . --config $(CONFIG)

# Запуск
server:
			@./$(BUILD_DIR)/pgw_server/$(TARGET_EXEC1)

client:
			@./$(BUILD_DIR)/pgw_client/$(TARGET_EXEC2)

# Запуск тестов
test:
			@./$(BUILD_DIR)/tests/test_config
			@./$(BUILD_DIR)/tests/test_logger
			@./$(BUILD_DIR)/tests/test_session_manager
			@./$(BUILD_DIR)/tests/test_udp_server
			@./$(BUILD_DIR)/tests/test_integration

# Очистить сборку
clean:
			@rm -rf $(BUILD_DIR)

# Полная пересборка
rebuild:	clean all

# Справка
help:
			@echo "Доступные команды:"
			@echo "  make           - Собрать проект (Release)"
			@echo "  make configure	- Собрать конфигурацию CMake"
			@echo "  make server    - Запустить сервер"
			@echo "  make client    - Запустить клиент"
			@echo "  make test      - Запустить тесты"
			@echo "  make clean     - Удалить build директорию"
			@echo "  make rebuild   - Полная пересборка"
			@echo "  make help      - Показать эту справку"
