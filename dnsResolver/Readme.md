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

### Итеративное разрешение DNS

DNS резолвер использует итеративный алгоритм разрешения доменных имен, начиная с корневых серверов:

1. **Запрос к корневым серверам**: Отправляется запрос к одному из 13 корневых серверов
2. **Получение NS записей**: Сервер возвращает список авторитетных серверов для следующего уровня
3. **Резолвинг серверов**: Получение IP адресов найденных серверов имен
4. **Переход к следующему уровню**: Запрос к серверам следующего уровня иерархии
5. **Повторение процесса**: Продолжение до получения конечных IP адресов

### Детальные этапы для google.com:

```
Домен: google.com

1. Запрос к корневому серверу (199.7.91.13)
   → Получение списка .com серверов (13 записей)

2. Запрос к .com серверу (192.5.6.30)
   → Получение списка Google серверов (ns1, ns2, ns3, ns4)

3. Резолвинг IP адресов серверов имен
   → Получение IP адресов (216.239.34.10, 216.239.32.10, и др.)

4. Запрос к Google серверу (216.239.34.10)
   → Получение конечных IP адресов (209.85.233.102, 209.85.233.139, и др.)

5. Возврат результата пользователю
```

### Особенности реализации:

- **Системные вызовы**: Использует `socket()`, `sendto()`, `recvfrom()`, `close()` через FileDesc
- **UDP протокол**: DNS запросы отправляются по UDP на порт 53
- **Рандомизация**: Случайный выбор серверов для балансировки нагрузки
- **Таймауты**: Настраиваемое время ожидания ответа (5 секунд)
- **Обработка сжатия**: Поддержка DNS сжатия для оптимизации трафика
- **Рекурсивная глубина**: Ограничение на 10 уровней для предотвращения зацикливания

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
