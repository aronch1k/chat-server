#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include "database.h"

#define MAX_CLIENTS 64

typedef struct ClientNode ClientNode;

typedef struct {
    int            fd;            /* дескриптор сокета */
    char           username[64];  /* пусто = не авторизован */
    int            authenticated;
    Database      *db;
    ClientNode   **client_list;   /* указатель на глобальный список */
    pthread_mutex_t *list_mutex;
} ClientContext;

struct ClientNode {
    ClientContext *ctx;
    ClientNode    *next;
};

/* Поток на каждого клиента */
void *client_thread(void *arg);

/* Разослать сообщение всем кроме sender_fd */
void broadcast(ClientNode *list, pthread_mutex_t *mtx,
               const char *msg, int sender_fd);

/* Добавить / удалить из списка */
void client_list_add   (ClientNode **head, pthread_mutex_t *mtx,
                        ClientContext *ctx);
void client_list_remove(ClientNode **head, pthread_mutex_t *mtx,
                        ClientContext *ctx);

#endif