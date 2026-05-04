#include "client.h"
#include "protocol.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

/* ---------- список клиентов ---------- */

void client_list_add(ClientNode **head, pthread_mutex_t *mtx,
                     ClientContext *ctx) {
    ClientNode *node = malloc(sizeof(ClientNode));
    node->ctx  = ctx;

    pthread_mutex_lock(mtx);
    node->next = *head;
    *head      = node;
    pthread_mutex_unlock(mtx);

    log_event(LOG_INFO, "Client added to list (fd=%d)", ctx->fd);
}

void client_list_remove(ClientNode **head, pthread_mutex_t *mtx,
                        ClientContext *ctx) {
    pthread_mutex_lock(mtx);
    ClientNode **cur = head;

    while (*cur) {
        if ((*cur)->ctx == ctx) {
            ClientNode *tmp = *cur;
            *cur = tmp->next;
            free(tmp);
            break;
        }
        cur = &(*cur)->next;
    }

    pthread_mutex_unlock(mtx);

    log_event(LOG_INFO, "Client removed from list (fd=%d)", ctx->fd);
}

/* ---------- broadcast ---------- */

void broadcast(ClientNode *list, pthread_mutex_t *mtx,
               const char *msg, int sender_fd) {
    pthread_mutex_lock(mtx);

    for (ClientNode *n = list; n; n = n->next) {
        if (n->ctx->fd != sender_fd && n->ctx->authenticated) {
            send(n->ctx->fd, msg, strlen(msg), MSG_NOSIGNAL);
        }
    }

    pthread_mutex_unlock(mtx);

    log_event(LOG_INFO, "Broadcast message from fd=%d", sender_fd);
}

/* ---------- send packet ---------- */

static void send_pkt(int fd, const char *cmd,
                     const char *a1, const char *a2) {
    char buf[MAX_PKT_LEN];
    build_packet(buf, sizeof(buf), cmd, a1, a2);
    send(fd, buf, strlen(buf), MSG_NOSIGNAL);
}

/* ---------- история ---------- */

typedef struct { int fd; } HistoryCbData;

static void history_cb(const char *username,
                       const char *message,
                       void *userdata) {
    HistoryCbData *d = userdata;
    send_pkt(d->fd, RESP_HISTORY, username, message);
}

/* ---------- поток клиента ---------- */

void *client_thread(void *arg) {
    ClientContext *ctx = arg;
    char buf[MAX_PKT_LEN];
    Packet pkt;

    log_event(LOG_INFO, "Client thread started (fd=%d)", ctx->fd);

    send_pkt(ctx->fd, RESP_INFO, "Welcome! Use REGISTER or LOGIN.", NULL);

    while (1) {
        ssize_t n = recv(ctx->fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;

        buf[n] = '\0';

        if (parse_packet(buf, &pkt) != 0) {
            log_event(LOG_WARN, "Bad packet from fd=%d", ctx->fd);
            send_pkt(ctx->fd, RESP_ERROR, "Bad packet format", NULL);
            continue;
        }

        /* REGISTER */
        if (strcmp(pkt.command, CMD_REGISTER) == 0) {
            int rc = db_register_user(ctx->db, pkt.arg1, pkt.arg2);

            if (rc == 0) {
                log_event(LOG_INFO, "User registered: %s", pkt.arg1);
                send_pkt(ctx->fd, RESP_OK, "Registered", NULL);
            } else {
                log_event(LOG_WARN, "Register failed: %s", pkt.arg1);
                send_pkt(ctx->fd, RESP_ERROR, "Register failed", NULL);
            }
        }

        /* LOGIN */
        else if (strcmp(pkt.command, CMD_LOGIN) == 0) {
            if (db_login_user(ctx->db, pkt.arg1, pkt.arg2) == 0) {
                strncpy(ctx->username, pkt.arg1, sizeof(ctx->username)-1);
                ctx->authenticated = 1;

                log_event(LOG_INFO, "User logged in: %s", ctx->username);

                send_pkt(ctx->fd, RESP_OK, "Logged in", NULL);

                HistoryCbData hd = { ctx->fd };
                db_get_history(ctx->db, 20, history_cb, &hd);

            } else {
                log_event(LOG_WARN, "Login failed: %s", pkt.arg1);
                send_pkt(ctx->fd, RESP_ERROR, "Wrong credentials", NULL);
            }
        }

        /* MESSAGE */
        else if (strcmp(pkt.command, CMD_MSG) == 0) {
            if (!ctx->authenticated) continue;

            db_save_message(ctx->db, ctx->username, pkt.arg1);


            log_event(LOG_INFO, "Message from %s (%zu bytes)",
                      ctx->username, strlen(pkt.arg1));

            char pktbuf[MAX_PKT_LEN];
            build_packet(pktbuf, sizeof(pktbuf),
                         RESP_MSG, ctx->username, pkt.arg1);

            send(ctx->fd, pktbuf, strlen(pktbuf), MSG_NOSIGNAL);
            broadcast(*ctx->client_list, ctx->list_mutex,
                      pktbuf, ctx->fd);
        }

        /* HISTORY */
        else if (strcmp(pkt.command, CMD_HISTORY) == 0) {
            HistoryCbData hd = { ctx->fd };
            db_get_history(ctx->db, 50, history_cb, &hd);

            log_event(LOG_INFO, "History requested by %s", ctx->username);
        }

        /* QUIT */
        else if (strcmp(pkt.command, CMD_QUIT) == 0) {
            log_event(LOG_INFO, "Client requested quit (%s)", ctx->username);
            break;
        }
    }

    if (ctx->authenticated) {
        log_event(LOG_INFO, "Client disconnected: %s", ctx->username);
    } else {
        log_event(LOG_INFO, "Client disconnected (unauthenticated, fd=%d)", ctx->fd);
    }

    client_list_remove(ctx->client_list, ctx->list_mutex, ctx);
    close(ctx->fd);
    free(ctx);

    return NULL;
}