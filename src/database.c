#include "database.h"
#include <stdio.h>
#include <string.h>

int db_init(Database *d, const char *path) {
    if (sqlite3_open(path, &d->db) != SQLITE_OK) {
        fprintf(stderr, "db_init: %s\n", sqlite3_errmsg(d->db));
        return -1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT    NOT NULL UNIQUE,"
        "  password TEXT    NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username  TEXT    NOT NULL,"
        "  message   TEXT    NOT NULL,"
        "  ts        DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    char *err = NULL;
    if (sqlite3_exec(d->db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "db_init tables: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

void db_close(Database *d) {
    if (d->db) sqlite3_close(d->db);
}

int db_register_user(Database *d,
                     const char *username,
                     const char *password) {
    const char *sql =
        "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(d->db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -2;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_CONSTRAINT) return -1; /* логин занят */
    if (rc != SQLITE_DONE)       return -2;
    return 0;
}

int db_login_user(Database *d,
                  const char *username,
                  const char *password) {
    const char *sql =
        "SELECT id FROM users WHERE username=? AND password=? LIMIT 1;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(d->db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_ROW) ? 0 : -1;
}

int db_save_message(Database *d,
                    const char *username,
                    const char *message) {
    const char *sql =
        "INSERT INTO messages (username, message) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(d->db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, message,  -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_history(Database *d, int limit,
                   msg_callback cb, void *userdata) {
    /*
     * Берём последние N строк в обратном порядке, потом разворачиваем,
     * чтобы клиент получил их от старых к новым.
     */
    const char *sql =
        "SELECT username, message FROM ("
        "  SELECT username, message, ts"
        "  FROM messages ORDER BY ts DESC LIMIT ?"
        ") ORDER BY ts ASC;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(d->db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *u = (const char *)sqlite3_column_text(stmt, 0);
        const char *m = (const char *)sqlite3_column_text(stmt, 1);
        if (cb) cb(u, m, userdata);
    }
    sqlite3_finalize(stmt);
    return 0;
}