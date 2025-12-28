# SMTP Client

SMTP-клиент для отправки электронных писем через SMTP-сервер.

## Компиляция

```bash
cd /home/bogdan.krasilnikov/projects/computer-networks
cmake --build build
```

## Использование

```bash
./build/smtpClient/smtp-client <server> <port> <from> <to> <subject> <body>
```

### Параметры:
- `server` - адрес SMTP-сервера
- `port` - порт SMTP-сервера (25, 587, 465)
- `from` - адрес отправителя
- `to` - адрес получателя  
- `subject` - тема письма
- `body` - текст письма

```bash
./build/smtpClient/smtp-client smtp.gmail.com 587 sender@gmail.com receiver@example.com "Test Subject" "Hello World!"
./build/smtpClient/smtp-client smtp.mail.ru 25 sender@mail.ru receiver@example.com "Test Subject" "Hello World!"
./build/smtpClient/smtp-client smtp.outlook.com 587 sender@outlook.com receiver@example.com "Test Subject" "Hello World!"
```

### Популярные SMTP серверы:

| Сервис | Сервер | Порт | Безопасность |
|--------|--------|------|--------------|
| Gmail | `smtp.gmail.com` | 587 | STARTTLS |
| Mail.ru | `smtp.mail.ru` | 25/587 | STARTTLS |
| Yandex | `smtp.yandex.ru` | 587 | STARTTLS |
| Outlook | `smtp.outlook.com` | 587 | STARTTLS |
| Yahoo | `smtp.mail.yahoo.com` | 587 | STARTTLS |

## Тестирование с локальным сервером

1. Запустите локальный SMTP сервер:
```bash
`python3 smtpClient/test_smtp_server.py`
```

2. В отдельном терминале отправьте письмо:
```bash
./build/smtpClient/smtp-client localhost 1025 sender@test.com receiver@test.com "Test Subject" "Test Message"
```

## Реализованные возможности

- ✅ Установление TCP-соединения с SMTP-сервером
- ✅ Получение приветствия сервера (220)
- ✅ Отправка команды HELO
- ✅ Отправка команды MAIL FROM
- ✅ Отправка команды RCPT TO
- ✅ Отправка команды DATA
- ✅ Передача содержимого письма
- ✅ Завершение сессии командой QUIT
- ✅ Обработка ошибок и кодов ответов SMTP

## Архитектура

Клиент использует готовые RAII-обёртки из директории `lib/`:
- `Connection` - для TCP соединений
- `FileDesc` - для файловых дескрипторов

Все ресурсы автоматически управляются через RAII, что обеспечивает безопасность и надёжность.
