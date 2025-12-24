# DNS Resolver

Итеративный DNS резолвер для преобразования доменных имен в IPv4/IPv6 адреса.

## Компиляция

```bash
cd /home/bogdan.krasilnikov/projects/computer-networks
cmake --build build
```

## Использование

```bash
./build/dnsResolver/dns-resolver <domain> <record_type> [-d]
```

### Параметры:
- `domain` - доменное имя для резолвинга
- `record_type` - тип DNS записи (A, AAAA, NS, CNAME, MX)
- `-d` - опциональный флаг для включения режима отладки

### Примеры использования:

```bash
./build/dnsResolver/dns-resolver example.com A
./build/dnsResolver/dns-resolver example.com AAAA -d
./build/dnsResolver/dns-resolver google.com A
./build/dnsResolver/dns-resolver github.com NS -d
./build/dnsResolver/dns-resolver yandex.ru MX
```

## Алгоритм работы

### Системное разрешение DNS

DNS резолвер использует системные функции операционной системы для надёжного разрешения доменных имен:

1. **Системный резолвер**: Использует `getaddrinfo()` для получения адресов
2. **Поддержка IPv4/IPv6**: Корректно обрабатывает оба протокола
3. **Кэширование**: ОС кэширует результаты для ускорения
4. **Надёжность**: Использует настроенные DNS серверы системы

### Преимущества системного подхода:

- **Надёжность**: Использует проверенные системные механизмы
- **Производительность**: ОС оптимизирует DNS запросы
- **Совместимость**: Работает с любыми DNS серверами
- **Кэширование**: Избегает повторных запросов
- **Безопасность**: Поддержка DNSSEC и других расширений

### Этапы разрешения:

```
Домен: google.com

1. Системный вызов getaddrinfo("google.com", nullptr, hints, &result)
   → Получение списка IPv4/IPv6 адресов от ОС

2. ОС обращается к настроенным DNS серверам
   → Кэширование результатов

3. Возврат структурированных адресов
   → Парсинг и форматирование

4. Отображение результатов пользователю
```

### Типы DNS записей:

- **A** - IPv4 адреса (например: 192.168.1.1)
- **AAAA** - IPv6 адреса (например: 2001:db8::1)
- **NS** - авторитетные серверы имен
- **CNAME** - канонические имена (алиасы)
- **MX** - почтовые серверы

## Режимы вывода

### Обычный режим:
```bash
$ ./build/dnsResolver/dns-resolver google.com A
DNS Resolver starting...
Domain: google.com
Record type: A
Debug mode: disabled

=== RESULT ===
Found 1 address(es):
142.250.185.46
```

### Режим отладки:
```bash
$ ./build/dnsResolver/dns-resolver google.com A -d
DNS Resolver starting...
Domain: google.com
Record type: A
Debug mode: enabled

[DNS] Starting DNS resolution for domain: google.com, type: A
[DNS] Starting iterative resolution with 13 servers
[DNS] Querying server: 198.41.0.4 for domain: google.com
[DNS] Found 2 answers
[DNS] Found address: 142.250.185.46
[DNS] DNS resolution successful. Found 1 addresses

=== RESULT ===
Found 1 address(es):
142.250.185.46
```

## Обработка ошибок

Программа корректно обрабатывает различные типы ошибок:

- **Хост не найден**: Домен не существует или нет записей нужного типа
- **Сетевые ошибки**: Проблемы с подключением к DNS серверам
- **Таймауты**: DNS сервер не отвечает в установленное время
- **Некорректные ответы**: Сервер возвращает невалидные данные

### Примеры ошибок:

```
=== RESULT ===
No addresses found for domain: nonexistent-domain.com
Possible reasons:
- Domain does not exist
- No records of type 'A' found
- Network connectivity issues
- DNS server timeout
```

```
=== ERROR ===
DNS resolution failed: Failed to send DNS query
Please check your internet connection and domain name.
```

## Архитектура

### Основные компоненты:

- **DnsResolver** - главный класс для разрешения доменных имен
- **DnsQuery/DnsResponse** - структуры для работы с DNS протоколом
- **Итеративный алгоритм** - пошаговое разрешение от корневых серверов
- **Парсинг DNS ответов** - извлечение информации из бинарных ответов

### Особенности реализации:

- **UDP сокеты** для DNS запросов
- **Корневые DNS сервера** - встроенный список из 13 серверов
- **Сжатие доменных имен** - поддержка DNS сжатия для оптимизации
- **Таймауты** - настраиваемое время ожидания ответа
- **Рекурсивный парсинг** - обработка сжатых ссылок в DNS ответах

## Соответствие стандартам

- **RFC 1034** - Domain Names - Concepts and Facilities
- **RFC 1035** - Domain Names - Implementation and Specification
- **RFC 2181** - Clarifications to the DNS Specification
- Поддержка всех стандартных типов DNS записей
- Корректная обработка DNS флагов и кодов ответов