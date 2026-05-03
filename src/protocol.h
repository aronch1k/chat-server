#include <stddef.h>
#ifndef PROTOCOL_H
#define PROTOCOL_H

/* Максимальные размеры */
#define MAX_MSG_LEN   1024
#define MAX_NAME_LEN  64
#define MAX_PASS_LEN  64
#define MAX_PKT_LEN   (MAX_MSG_LEN + MAX_NAME_LEN + 32)

/* Команды клиент -> сервер */
#define CMD_REGISTER  "REGISTER"
#define CMD_LOGIN     "LOGIN"
#define CMD_MSG       "MSG"
#define CMD_HISTORY   "HISTORY"
#define CMD_QUIT      "QUIT"

/* Ответы сервер -> клиент */
#define RESP_OK       "OK"
#define RESP_ERROR    "ERROR"
#define RESP_MSG      "MSG"
#define RESP_INFO     "INFO"
#define RESP_HISTORY  "HISTORY"

typedef struct {
    char command[32];
    char arg1[MAX_NAME_LEN];
    char arg2[MAX_MSG_LEN];
} Packet;

/* Разбирает строку "CMD|arg1|arg2\n" -> Packet */
int  parse_packet(const char *raw, Packet *pkt);

/* Формирует строку для отправки */
int  build_packet(char *buf, size_t bufsz,
                  const char *cmd,
                  const char *arg1,
                  const char *arg2);

#endif