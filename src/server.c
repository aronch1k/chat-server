#include "server.h"
#include "client.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

static volatile int running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
    printf("\n[server] Shutting down...\n");
}

int server_run(const ServerConfig *cfg) {
    signal(SIGINT,  handle_sigint);
    signal(SIGPIPE, SIG_IGN); /* игнорируем SIGPIPE при записи в закрытый сокет */

    /* --- Инициализация БД --- */
    Database db;
    if (db_init(&db, cfg->db_path) != 0) {
        fprintf(stderr, "[server] Failed to open database.\n");
        return -1;
    }
    printf("[server] Database: %s\n", cfg->db_path);

    /* --- Список клиентов --- */
    ClientNode    *client_list = NULL;
    pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

    /* --- TCP-сокет --- */
    int srv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_fd < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(cfg->port);

    if (bind(srv_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(srv_fd); return -1;
    }
    if (listen(srv_fd, BACKLOG) < 0) {
        perror("listen"); close(srv_fd); return -1;
    }

    printf("[server] Listening on port %d...\n", cfg->port);

    /* --- Accept loop --- */
    while (running) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int cli_fd = accept(srv_fd,
                            (struct sockaddr *)&cli_addr, &cli_len);
        if (cli_fd < 0) {
            if (running) perror("accept");
            break;
        }

        printf("[server] New connection from %s:%d\n",
               inet_ntoa(cli_addr.sin_addr),
               ntohs(cli_addr.sin_port));

        ClientContext *ctx = calloc(1, sizeof(ClientContext));
        ctx->fd          = cli_fd;
        ctx->db          = &db;
        ctx->client_list = &client_list;
        ctx->list_mutex  = &list_mutex;

        client_list_add(&client_list, &list_mutex, ctx);

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&tid, &attr, client_thread, ctx);
        pthread_attr_destroy(&attr);
    }

    close(srv_fd);
    db_close(&db);
    return 0;
}