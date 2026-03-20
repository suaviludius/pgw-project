# ------------ Этап 1: Сборка проекта -------------
FROM ubuntu:22.04 AS builder

# Аргументы для прередачи в CMake
ARG BUILD_TESTS=ON
ARG BUILD_TYPE=Release

# Устанавливаем необходимые пакеты
# Одно использование RUN = 1 слой
# rm ... - удаляет кэш после выполненых в RUN команд
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Устанавливаем рабочую директорию
WORKDIR /build

# Копируем ТОЛЬКО файлы сборки (для кэширования)
COPY CMakeLists.txt ./
COPY Makefile ./
# Копируем исходный код (при изменении кода кэш сбросится после этого шага)
COPY include/ include/
COPY src/ src/
COPY pgw_client/ pgw_client/
COPY pgw_server/ pgw_server/
COPY tests/ tests/
COPY configs/ configs/

# Конфигурируем и собираем
# Не использую make команды, чтобы иметь лучший контроль
# -S - source dirrectory ( . - текущая)
# -B - build dirrectory ( . - текущая)
# Такой подход будет собирать в билде и исходники и билд от cmake
RUN cmake -S . -B . \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_TESTS=${BUILD_TESTS} \
    -DBUILD_SERVER=ON \
    -DBUILD_CLIENT=ON

# Концовочка дает распараллелить загрузку
RUN cmake --build . --config ${BUILD_TYPE} -j$(nproc)

# ------ Этап 2: Финальный образ для запуска ------
FROM ubuntu:22.04 AS runtime

# Устанавливаем минимальные зависимости для запуска (не компиляции)
# libstdc++6 - стандартная библиотека C++
# libgcc-s1  - GCC
# curl       - http
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    libgcc-s1 \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Создаем пользователя для запуска приложения
# -m        - cоздать домашнюю директорию для пользователя
# (в ней будут временные файлы)
# -u 1000   - задать конкретный UID (User ID) = 1000
# (это ID домашнего пользвателя, что позволит не использовать
# sudo для чтения/изменения файлов в системе)
# pgwuser   - назвались груздем
RUN useradd -m -u 1000 pgwuser

# Устанавливаем рабочую директорию
# Дальше команды выполняются в ней
WORKDIR /app

# Копируем скомпилированные бинарники из builder
# Поэтому рантайм и будет легче, у него ТОЛЬКО бинарники
COPY --from=builder /build/bin/pgw_server /app/
COPY --from=builder /build/bin/pgw_client /app/

# Копируем конфигурационные файлы
COPY configs/ /app/configs/

# Создаем директории для логов и CDR с правильными правами
RUN mkdir -p /app/logs /app/cdr && \
    chown pgwuser:pgwuser /app/logs /app/cdr

# Создаем директорию для временных файлов pgwuser
RUN mkdir -p /home/pgwuser && \
    chown pgwuser:pgwuser /home/pgwuser

# Переключаемся на не-root пользователя
# Команды будут выполняются от имени pgwuser
# Это обезопасит процесс внутри контейнера
USER pgwuser

# Заметка для IMAGES (при навелении мышки будет отображать)
# 9000 - UDP порт для сессий
# 8080 - HTTP порт для API
EXPOSE 9000/udp 8080/tcp

# Команда по умолчанию - запуск сервера
CMD ["./pgw_server", "configs/pgw_server.docker.json"]
