#include "server.h"
#include "client.h"
#include "database.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
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
    log_event(LOG_INFO, "SIGINT received, shutting down server...");
}

int server_run(const ServerConfig *cfg) {
    signal(SIGINT,  handle_sigint);
    signal(SIGPIPE, SIG_IGN);

    if (log_init(cfg->log_path) != 0) {
        fprintf(stderr, "Failed to open log file\n");
        return -1;
    }
    printf(">>> BEFORE log_init\n");
    log_init(cfg->log_path ? cfg->log_path : "server.log");
    printf(">>> AFTER log_init\n");
    
    log_event(LOG_INFO, "Server starting on port %d", cfg->port);
    log_event(LOG_INFO, "Database: %s", cfg->db_path);

    Database db;
    if (db_init(&db, cfg->db_path) != 0) {
        log_event(LOG_ERROR, "Database init failed");
        return -1;
    }

    ClientNode *client_list = NULL;
    pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

    int srv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_fd < 0) {
        log_event(LOG_ERROR, "Socket creation failed");
        return -1;
    }

    int opt = 1;
    setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(cfg->port);

    if (bind(srv_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_event(LOG_ERROR, "Bind failed");
        close(srv_fd);
        return -1;
    }

    if (listen(srv_fd, BACKLOG) < 0) {
        log_event(LOG_ERROR, "Listen failed");
        close(srv_fd);
        return -1;
    }

    log_event(LOG_INFO, "Server listening...");

    while (running) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);

        int cli_fd = accept(srv_fd,
                            (struct sockaddr *)&cli_addr, &cli_len);

        if (cli_fd < 0) {
            if (running)
                log_event(LOG_ERROR, "Accept failed");
            break;
        }

        const char *ip = inet_ntoa(cli_addr.sin_addr);
        int port = ntohs(cli_addr.sin_port);

        log_event(LOG_INFO, "New connection from %s:%d", ip, port);

        ClientContext *ctx = calloc(1, sizeof(ClientContext));
        if (!ctx) {
            log_event(LOG_ERROR, "Memory allocation failed");
            close(cli_fd);
            continue;
        }

        ctx->fd = cli_fd;
        ctx->db = &db;
        ctx->client_list = &client_list;
        ctx->list_mutex = &list_mutex;

        client_list_add(&client_list, &list_mutex, ctx);

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&tid, &attr, client_thread, ctx) != 0) {
            log_event(LOG_ERROR, "Thread creation failed");
            close(cli_fd);
        }

        pthread_attr_destroy(&attr);
    }

    log_event(LOG_INFO, "Server shutting down");

    close(srv_fd);
    db_close(&db);
    log_close();

    return 0;
}