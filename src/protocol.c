#include "protocol.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>

int parse_packet(const char *raw, Packet *pkt) {
    if (!raw || !pkt) return -1;
    memset(pkt, 0, sizeof(*pkt));

    char buf[MAX_PKT_LEN];
    strncpy(buf, raw, sizeof(buf) - 1);

    /* Убираем \n и \r */
    size_t len = strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
        buf[--len] = '\0';

    char *token = strtok(buf, "|");
    if (!token) return -1;
    strncpy(pkt->command, token, sizeof(pkt->command) - 1);

    token = strtok(NULL, "|");
    if (token) strncpy(pkt->arg1, token, sizeof(pkt->arg1) - 1);

    token = strtok(NULL, "|");
    if (token) strncpy(pkt->arg2, token, sizeof(pkt->arg2) - 1);

    return 0;
}

int build_packet(char *buf, size_t bufsz,
                 const char *cmd,
                 const char *arg1,
                 const char *arg2) {
    if (!buf || !cmd) return -1;
    if (arg1 && arg2)
        snprintf(buf, bufsz, "%s|%s|%s\n", cmd, arg1, arg2);
    else if (arg1)
        snprintf(buf, bufsz, "%s|%s\n", cmd, arg1);
    else
        snprintf(buf, bufsz, "%s\n", cmd);
    return 0;
}