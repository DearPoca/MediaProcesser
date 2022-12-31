#include "logger.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

const int log_level = INFO;

const char *LogLevel2Str[] = {"DEBUG", "INFO", "WARN", "ERROR"};

void LOG(const char *func, const char *filename, int line, int level, const char *format, ...) {
    if (level < log_level) return;

    char log_buffer[MAX_LOG_STR_SIZE];

    time_t now = time(0);
    strftime(log_buffer, sizeof(log_buffer), "[%Y-%m-%d %H:%M:%S]", localtime(&now));

    sprintf(log_buffer + strlen(log_buffer), "[%s][%s:%d][%s]", LogLevel2Str[level], filename, line, func);

    va_list ap;
    va_start(ap, format);
    vsnprintf(log_buffer + strlen(log_buffer), MAX_LOG_STR_SIZE, format, ap);

    printf("%s\n", log_buffer);
}