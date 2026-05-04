#include "log.h"
#include <time.h>
#include <stdarg.h>

static FILE *log_file = NULL;

static const char *level_str(LogLevel lvl) {
    switch (lvl) {
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNK";
    }
}

int log_init(const char *path) {
    log_file = fopen(path ? path : "server.log", "a");
    return log_file ? 0 : -1;
}

void log_close() {
    if (log_file) fclose(log_file);
}

void log_event(LogLevel level, const char *fmt, ...) {
    if (!log_file) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    va_list args;
    va_start(args, fmt);

    printf("[%s] [%s] ", time_buf, level_str(level));
    vprintf(fmt, args);
    printf("\n");

    fprintf(log_file, "[%s] [%s] ", time_buf, level_str(level));
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");
    fflush(log_file);

    va_end(args);
}