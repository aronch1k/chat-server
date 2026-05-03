# Chat Server

Серверная часть сетевого чата. Язык: C. БД: SQLite. ОС: Ubuntu.

## Сборка

Bash
sudo apt install -y gcc make libsqlite3-dev
make


## Запуск

Bash
./chat-server                        # порт 9090, БД chat.db
./chat-server --port 8080 --db /tmp/chat.db


## Протокол (TCP, текстовый)

| Команда клиента          | Ответ сервера              |
|--------------------------|----------------------------|
| REGISTER\|user\|pass   | OK\|... или ERROR\|... |
| LOGIN\|user\|pass      | OK\|... + история        |
| MSG\|text              | MSG\|user\|text (всем)   |
| HISTORY                | N строк HISTORY\|user\|msg |
| QUIT                   | разрыв соединения          |

## Быстрый тест

Bash
# Терминал 1 — сервер
./chat-server

# Терминал 2 — клиент (telnet или netcat)
nc localhost 9090
REGISTER|alice|secret
LOGIN|alice|secret
MSG|Привет всем!
QUIT