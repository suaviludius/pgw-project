---
created: 2026-04-01T12:57:13.172Z
updated: 2026-04-03T16:39:53.420Z
assigned: ""
progress: 1
tags: []
due: 2026-04-02T00:00:00.000Z
started: 2026-04-01T00:00:00.000Z
completed: 2026-04-02T00:00:00.000Z
---

# [REFACTOR] Редактирование программы в соответсвии с принципами SOLID

S — Single Responsibility Principle
O — Open/Closed Principle
L — Liskov Substitution Principle
I — Interface Segregation Principle
D — Dependency Inversion Principle

## Sub-tasks

- [x] Создать класс Application для управления жизненным циклом приложения
- [x] Выделить инициализацию логгера в отдельный метод
- [x] Выделить инициализацию базы данных в отдельный метод
- [x] Выделить инициализацию CDR Writer в отдельный метод
- [x] Выделить инициализацию серверов в отдельный метод
- [x] Рефакторинг main.cpp с использованием новых модулей
- [x] Метод addrToString()  вынести в SocketUtils  и удалить из интерфеса сокета
- [x] Прогнать все тесты, убедиться в своей гениальности
