---
created: 2026-03-23T13:04:05.891Z
updated: 2026-03-24T21:39:31.911Z
assigned: ""
progress: 0.6
tags: []
due: 2026-03-30T00:00:00.000Z
started: 2026-03-23T00:00:00.000Z
---

# [FEAT] SQLite интеграция

Замена файлового CdrWriter на SQLite базу данных для хранения CDR-записей и событий системы

## Sub-tasks

- [x] Спроектировать схему БД
- [x] Создать SQL скрипт схемы
- [ ] Реализовать DatabaseManager класс (подключение, инициализация)
- [x] Переписать CdrWriter на запись в SQLite
- [ ] Добавить класс EventLogger для записи системных событий
- [x] Миграция CdrWriter с файлов на SQLite
- [x] Написать unit-тесты для DatabaseManager
- [ ] Написать unit-тесты для  EventLogger
