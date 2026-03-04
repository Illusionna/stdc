#ifndef _LOG_H_
#define _LOG_H_


#include <time.h>
#include <stdio.h>
#include <stdarg.h>


extern const char *TIPS[6];
enum {LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL};


#define MAX_CALLBACKS 64


#define log_trace(...) __log_print__(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) __log_print__(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) __log_print__(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warning(...) __log_print__(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) __log_print__(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) __log_print__(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)


typedef struct {
    va_list argument_pointer;
    char *fmt;
    char *file;
    struct tm *time;
    void *ctx;
    int line;
    int level;
} LogEvent;


typedef void (*_LogCallbackFunc)(LogEvent *event);
typedef void (*_LogLockFunc)(int lock, void *ctx);


typedef struct {
    _LogCallbackFunc func;
    void *ctx;
} _LogCallback;


/**
 * @brief Configure the log written to the disk.
 * @param f The pointer of log file.
**/
void log_config_write(FILE *f);


/**
 * @brief Set the log mode.
 * @param mode `1` for not printing, `0` for default printing.
**/
void log_setting(int mode);


/**
 * @brief Add a custom callback function.
 * @param func Callback function (go to the `log.h` declaration to see how to use it).
 * @param ctx Store the user's context data and return it directly after the callback is executed.
 * @example
 * @code
 * void custom_callback(LogEvent *event) {
 *     printf("I want to write to a CSV file.\n");
 *     fprintf(event->ctx, "This, is, me, %s\n", "Illusionna");
 *     vfprintf(event->ctx, event->fmt, event->argument_pointer);
 * }
 * FILE *fp = fopen("demo.csv", "w");
 * log_add_callback(custom_callback, fp);
 * @endcode
**/
void log_add_callback(_LogCallbackFunc func, void *ctx);


/**
 * @brief Configure the multi-thread lock for the log.
 * @param func Thread lock function (go to the `log.h` declaration to see how to use it).
 * @param ctx A lock (e.g., a `Mutex`).
 * @example
 * @code
 * void mutex_func(int lock, void *ctx) {
 *     Mutex *mutex = (Mutex *)ctx;
 *     if (lock) {
 *         printf("-------- lock --------\n");
 *         mutex_lock(mutex);
 *     } else {
 *         mutex_unlock(mutex);
 *         printf("------- unlock -------\n");
 *     }
 * }
 * log_config_thread_lock(mutex_func, &mutex);
 * @endcode
**/
void log_config_thread_lock(_LogLockFunc func, void *ctx);


/**
 * @brief Print a log to terminal.
 * @param level The level of log (`LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`, `LOG_FATAL`).
 * @param file The C language file.
 * @param line The C language file line.
 * @param fmt The content string.
**/
void __log_print__(int level, char *file, int line, char *fmt, ...);


/**
 * @brief Initialize the log event.
 * @param event The event of log.
 * @param ctx Store the user's context data and return it directly after the callback is executed.
**/
void __log_init_event__(LogEvent *event, void *ctx);


/**
 * @brief Log enables thread lock.
**/
void __log_lock__();


/**
 * @brief Log disabling thread lock.
**/
void __log_unlock__();


/**
 * @brief Standard output callback function.
 * @param event The event of log.
**/
void __log_callback_stdout__(LogEvent *event);


/**
 * @brief Write to the disk file callback function.
 * @param event The event of log.
**/
void __log_callback_write__(LogEvent *event);


#endif
