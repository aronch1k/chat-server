#ifndef LOG_H
#define LOG_H

#include <stdio.h>


typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

int  log_init(const char *path);
void log_close();
void log_event(LogLevel level, const char *fmt, ...);

#endif