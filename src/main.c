#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog) {
    printf("Usage: %s [--port PORT] [--db PATH]\n", prog);
    printf("  --port PORT   TCP port (default: %d)\n", DEFAULT_PORT);
    printf("  --db   PATH   SQLite DB path (default: %s)\n", DEFAULT_DB_PATH);
}

int main(int argc, char *argv[]) {
    ServerConfig cfg;
    cfg.port = DEFAULT_PORT;
    strncpy(cfg.db_path, DEFAULT_DB_PATH, sizeof(cfg.db_path) - 1);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i+1 < argc) {
            cfg.port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--db") == 0 && i+1 < argc) {
            strncpy(cfg.db_path, argv[++i], sizeof(cfg.db_path) - 1);
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]); return 0;
        }
    }

    return server_run(&cfg);
}