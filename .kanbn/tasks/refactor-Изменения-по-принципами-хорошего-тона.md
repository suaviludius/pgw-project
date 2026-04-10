---
created: 2026-04-02T12:08:42.687Z
updated: 2026-04-10T11:58:06.537Z
assigned: ""
progress: 0
tags: []
---

# [REFACTOR] Изменения по принципами хорошего тона

Don’t Repeat Yourself в целом пойдет, но всё еще есть проблемы

## Sub-tasks

- [ ] Выделить общий метод removeSession() в менеджере сессий
- [ ] Дублирование паттерна логирования в addFileSink() и addDatabaseSink(), сделать один addSink()
- [ ] Создать setResponse(res, status, content) для HttpServer в handleSubscriberCheck() и handleShutdown()
- [ ] Применить паттерн Builder для создания Application (чтобы случайно не обкакаться при неправильном порядке инициализации)
