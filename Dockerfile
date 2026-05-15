# ------------ Этап 1: Сборка проекта -------------
FROM ubuntu:22.04 AS builder

# Аргументы для прередачи в CMake
ARG BUILD_TYPE=Release

# Устанавливаем необходимые пакеты
# Одно использование RUN = 1 слой
# rm ... - удаляет кэш после выполненых в RUN команд
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsqlite3-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Устанавливаем рабочую директорию для файлов проекта
WORKDIR /project

# Копируем ТОЛЬКО файлы сборки (для кэширования)
COPY CMakeLists.txt ./
COPY Makefile ./
# Копируем исходный код (разделяем для еще лучшего кеширования)
COPY include/ ./include/
COPY src/ ./src/
COPY app/ ./app/
COPY tests/ ./tests/
COPY configs/ ./configs/

# В эту папку cmake будет собирать
WORKDIR /project/build

# Конфигурируем и собираем
# Не использую make команды, чтобы иметь лучший контроль
RUN cmake .. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_TESTS=ON \
    -DBUILD_SERVER=ON \
    -DBUILD_CLIENT=ON

# Собираем только нужные цели
RUN cmake --build . --config ${BUILD_TYPE} -j$(nproc)

# ------ Этап 2: Образ для сервера ------
FROM ubuntu:22.04 AS server

# Устанавливаем минимальные зависимости для запуска (не компиляции)
# libstdc++6 - стандартная библиотека C++
# libgcc-s1  - GCC
# curl       - http
# ca-certificates - если работаешь с защищенными соединениями (БД, HTTPS ...)
# libsqlite3-0  - SQLite runtime
# tzdata        - правильное отображение веремени
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    libgcc-s1 \
    curl \
    ca-certificates \
    libsqlite3-0 \
    libssl3 \
    tzdata \
    && rm -rf /var/lib/apt/lists/*

# 1) Создаем крутого пользователя
# -m        - cоздать /home директорию для пользователя (в ней будут временные файлы)
# -u 1000   - задать конкретный UID (User ID) = 1000 (этот ID позволит не использовать sudo для файлов в системе)
# pgwuser   - назвались
# 2) Создаем директории для временных файлов pgwuser
# 3) Даем доступ pgwuser к директории /app
RUN useradd -m -u 1000 -s /bin/bash pgwuser && \
    mkdir -p /app/configs /app/logs /app/cdr && \
    chown -R pgwuser:pgwuser /app

# Устанавливаем рабочую директорию
WORKDIR /app

# Копируем только бинарники из builder (поэтому образ рантайма будет лечгче)
COPY --from=builder /project/build/bin/pgw_server /app/pgw_server

# Копируем конфетки
COPY configs/ /app/configs/

# Исправляем права
RUN chmod +x /app/pgw_server && \
    chown -R pgwuser:pgwuser /app

# Переключаемся на нашего пользователя pgwuser.
# Выполнение команд не от root обесопасит работу в контейнере!
USER pgwuser

# Заметка для IMAGES (при навелении мышки будет отображать)
# 9000 - UDP порт для сессий
# 9090 - TCP порт для HMI
# 8080 - TCP порт для HTTP API
EXPOSE 9000/udp 9090/tcp 8080/tcp

# Точка входа
ENTRYPOINT ["./pgw_server"]
CMD ["configs/pgw_server.docker.json"]


# ------ Этап 3: Образ для клиента ------
FROM ubuntu:22.04 AS client

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    libgcc-s1 \
    tzdata \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -u 1000 -s /bin/bash pgwuser && \
    mkdir -p /app/configs /app/logs && \
    chown -R pgwuser:pgwuser /app

WORKDIR /app

COPY --from=builder /project/build/bin/pgw_client /app/pgw_client
COPY configs/ /app/configs/

RUN chmod +x /app/pgw_client && \
    chown -R pgwuser:pgwuser /app

USER pgwuser

ENTRYPOINT ["./pgw_client"]
CMD ["configs/pgw_client.docker.json", "001010123456789"]


# ------ Этап 4: Образ для тестирования ------
FROM ubuntu:22.04 AS testing

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    libgcc-s1 \
    ca-certificates \
    libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tests

COPY --from=builder /project/build/bin/ /tests/
COPY configs/ /tests/configs/

RUN chmod +x /tests/test_* && \
    mkdir -p /tests/logs

# Команда по умолчанию - запуск всех тестов
CMD ["sh", "-c", \
     "./test_config && \
      ./test_logger && \
      ./test_database && \
      ./test_udp_server && \
      ./test_tcp_serializer && \
      ./test_tcp_handler && \
      ./test_tcp_server && \
      ./test_integration"]