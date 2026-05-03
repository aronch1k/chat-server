#ifndef SERVER_H
#define SERVER_H

#define DEFAULT_PORT    9090
#define DEFAULT_DB_PATH "chat.db"
#define BACKLOG         10

typedef struct {
    int   port;
    char  db_path[256];
} ServerConfig;

int server_run(const ServerConfig *cfg);

#endif