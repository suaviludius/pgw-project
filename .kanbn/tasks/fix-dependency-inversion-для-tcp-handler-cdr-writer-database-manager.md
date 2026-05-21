---
created: 2026-05-21T09:33:12.225Z
updated: 2026-05-21T21:50:27.770Z
assigned: ""
progress: 0
tags: []
completed: 2026-05-21T21:50:27.770Z
---

# [FIX] Dependency Inversion для TcpHandler -> CdrWriter -> DatabaseManager

На данный момен происходит 
1) нарушение Layered Architecture - Handler (верхний слой) обращается напрямую к DatabaseManager (нижний слой), минуя CDR Writer (бизнес-логику) 
2) Нарушение Single Responsibility - TcpHandler знает о деталях реализации БД

## Sub-tasks

- [x] Внести метод getRecentCdr(), getRecordCount() в ICdrWriter (сейчас есть только в DatabaseManager)
- [x] Добавить методы getRecentRecords(), getRecordCount() в FileCdrWriter
- [x] Обновить TcpHandler (заменить IDatabaseManager на ICdrWriter)
- [x] Обновить Server (исправить параметры для конструктора TcpHandler)
- [x] Обновить моки и тесты, чтобы запускались
- [x] Внести изменения в схему классов README.md
