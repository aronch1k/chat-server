#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

typedef struct {
    sqlite3 *db;
} Database;

/* Инициализация — создаёт таблицы если их нет */
int  db_init(Database *d, const char *path);
void db_close(Database *d);

/* Регистрация: 0 = успех, -1 = занят логин, -2 = ошибка */
int  db_register_user(Database *d,
                      const char *username,
                      const char *password);

/* Авторизация: 0 = успех, -1 = неверный логин/пароль */
int  db_login_user(Database *d,
                   const char *username,
                   const char *password);

/* Сохранить сообщение */
int  db_save_message(Database *d,
                     const char *username,
                     const char *message);

/*
 * Получить последние N сообщений.
 * Вызывает callback(username, message, userdata) для каждого.
 */
typedef void (*msg_callback)(const char *username,
                              const char *message,
                              void *userdata);

int  db_get_history(Database *d, int limit,
                    msg_callback cb, void *userdata);

#endif