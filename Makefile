# Конфигурации
BUILD_DIR := build
CONFIG := Release
BUILD_TYPE := Release

# Имя исполняемого файла
TARGET_EXEC1 := pgw_server
TARGET_EXEC2 := pgw_client

# =============================
# Phony targets
# =============================
.PHONY: all configure build server client test clean rebuild help
.PHONY: docker-build docker-start docker-server docker-client docker-stop docker-test docker-clean docker-logs docker-shell

# =============================
# Commands
# =============================

# По умолчанию простая сборка
all:		configure build

# Конфигурация Cmake
# Если не удалять закешированные данные, то выставленные опции не будут применяться
configure:
	@rm -rf $(BUILD_DIR)/CMakeCache.txt $(BUILD_DIR)/CMakeFiles
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

#Сборка проекта
build:
	@cd $(BUILD_DIR) && cmake --build . --config $(CONFIG)

# Запуск
server:
	@./$(BUILD_DIR)/bin/$(TARGET_EXEC1) configs/pgw_server.json

client:
	@./$(BUILD_DIR)/bin/$(TARGET_EXEC2) configs/pgw_client.json

# Запуск тестов (Проверяем была ли сборка с тестами)
# && форма даст всем тестам выполниться, даже если один упал
test:
	@if [ ! -f "$(BUILD_DIR)/tests/test_config" ]; then \
		echo "Тесты не найдены! Соберите с BUILD_TESTS=ON"; \
		exit 1; \
	fi
	@./$(BUILD_DIR)/tests/test_config && \
		./$(BUILD_DIR)/tests/test_logger && \
		./$(BUILD_DIR)/tests/test_database && \
		./$(BUILD_DIR)/tests/test_session_manager && \
		./$(BUILD_DIR)/tests/test_udp_server && \
		./$(BUILD_DIR)/tests/test_tcp_serializer && \
		./$(BUILD_DIR)/tests/test_tcp_server && \
		./$(BUILD_DIR)/tests/test_integration

# Очистить сборку
clean:
	@rm -rf $(BUILD_DIR)

# Полная пересборка
rebuild: clean all


# =============================
# Docker
# =============================

DOCKER_IMAGE := pgw-project
DOCKER_TAG := latest

# Собрать Docker образ
docker-build:
	@docker build -t $(DOCKER_IMAGE):$(DOCKER_TAG) .

docker-build-t:
	@docker build --build-arg BUILD_TESTS=ON -t $(DOCKER_IMAGE):$(DOCKER_TAG) .

# Запустить через docker-compose (сервер + клиент)
# (если не было билда, то сам соберет)
docker-start:
	@docker compose up -d

# Запустить только сервер через docker-compose
docker-server:
	@docker compose up -d pgw_server

# Запустить только клиента через docker-compose
docker-client:
	@docker compose run --rm pgw_client

# Остановить docker-compose
docker-stop:
	@docker compose down

# Запустить тесты через docker-compose
docker-test:
	@docker compose --profile test run --rm test_runner

# Полная очистка docker-compose
docker-clean:
	@docker compose down
	@docker compose rm -f
	@docker volume prune -f

# Логирование docker-compose
docker-logs:
	@docker compose logs -f pgw_server

# Подключиться к контейнеру (shell)
docker-shell:
	@docker exec -it pgw_server /bin/bash

# =============================
# Справка
# =============================

help:
	@echo "	 Доступные команды:"
	@echo "  make           - Собрать проект (Release)"
	@echo "  make configure - Собрать конфигурацию CMake"
	@echo "  make server    - Запустить сервер"
	@echo "  make client    - Запустить клиент"
	@echo "  make test      - Запустить тесты"
	@echo "  make clean     - Удалить build директорию"
	@echo "  make rebuild   - Полная пересборка"
	@echo ""
	@echo "	 Docker команды:"
	@echo "  make docker-build     	- Собрать Docker образ без тестов"
	@echo "  make docker-build-t   	- Собрать Docker образ c тестами"
	@echo "  make docker-start   	- Собрать и запустить все сервисы (лучший вариант)"
	@echo "  make docker-server   	- Запустить только сервер"
	@echo "  make docker-client   	- Запустить только клиента"
	@echo "  make docker-test     	- Запустить тесты"
	@echo "  make docker-stop     	- Остановить все сервисы"
	@echo "  make docker-clean    	- Очистить ресурсы"
	@echo "  make docker-logs     	- Показать логи сервера"
	@echo "  make docker-shell    	- Подключиться к контейнеру"
