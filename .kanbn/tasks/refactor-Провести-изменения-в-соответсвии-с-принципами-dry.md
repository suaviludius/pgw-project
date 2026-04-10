---
created: 2026-04-02T12:08:42.687Z
updated: 2026-04-03T16:39:34.645Z
assigned: ""
progress: 0
tags: []
---

# [REFACTOR] Провести изменения в соответсвии с принципами DRY

Don’t Repeat Yourself в целом пойдет, но всё еще есть проблемы

## Sub-tasks

- [ ] Выделить общий метод removeSession() в менеджере сессий
- [ ] Дублирование паттерна логирования в addFileSink() и addDatabaseSink(), сделать один addSink()
- [ ] Создать setResponse(res, status, content) для HttpServer в handleSubscriberCheck() и handleShutdown()
