#pragma once

#include <cstring>

// Log levels
enum LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
};

#define MAX_LOG_STR_SIZE 1024
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define log_debug(format, ...) LOG(__func__, __FILENAME__, __LINE__, DEBUG, format, ##__VA_ARGS__)
#define log_info(format, ...) LOG(__func__, __FILENAME__, __LINE__, INFO, format, ##__VA_ARGS__)
#define log_warn(format, ...) LOG(__func__, __FILENAME__, __LINE__, WARN, format, ##__VA_ARGS__)
#define log_error(format, ...) LOG(__func__, __FILENAME__, __LINE__, ERROR, format, ##__VA_ARGS__)

void LOG(const char *func, const char *filename, int line, int level, const char *format, ...);