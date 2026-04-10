---
created: 2026-03-23T13:04:05.891Z
updated: 2026-04-01T13:09:46.337Z
assigned: ""
progress: 1
tags: []
due: 2026-03-30T00:00:00.000Z
started: 2026-03-23T00:00:00.000Z
completed: 2026-03-30T00:00:00.000Z
---

# [FEAT] SQLite интеграция

Замена файлового CdrWriter на SQLite базу данных для хранения CDR-записей и событий системы

## Sub-tasks

- [x] Спроектировать схему БД
- [x] Создать SQL скрипт схемы
- [x] Переписать CdrWriter на запись в SQLite (CdrFactory + File/Database CdrWriter)
- [x] Миграция CdrWriter с файлов на SQLite (main + session manager + tests)
- [x] Написать unit-тест для DatabaseManager
- [x] Отредактировать integration-тест
- [x] Переписать logger на запись в SQLite
- [x] Миграция logger с файлов на SQLite
- [x] Отредактировать integration-тест
- [x] Изменить формат конфиг файлов для работы с БД
