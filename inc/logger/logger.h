#ifndef LOGGER_H
#define LOGGER_H

// log level (the lower level, the higher proiority)
typedef enum {
    LOG_EMERGENCY,
    LOG_ALERT,
    LOG_CRITICAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
} logLevel;

// Function to initialize logger
void loggerInit(logLevel level, const char* fileName);
void loggerSetLevel(logLevel level);
void loggerClose(void);

// Logging functions
void loggerLog(logLevel level, const char* fileName, int line, const char* format, ...);

// macros
#define logMessage(level,...) loggerLog(level, __FILE__, __LINE__, __VA_ARGS__)

#endif // LOGGER_H