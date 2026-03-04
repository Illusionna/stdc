#ifndef _ASYNC_LOG_H_
#define _ASYNC_LOG_H_


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


#include "os.h"
#include "threadpool.h"


extern const char *TIPS[6];
extern const char *COLOURS[6];


enum {LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL};


#define ASYNC_LOG_MAX_CALLBACKS 64
#define ASYNC_LOG_MAX_MESSAGE_LENGTH 512
#define ASYNC_LOG_MAX_THREAD_POOL_SIZE 1024 // Maximum concurrent asynchronous logger count.


#define asynclog_trace(...) __async_log_print__(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define asynclog_debug(...) __async_log_print__(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define asynclog_info(...) __async_log_print__(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define asynclog_warning(...) __async_log_print__(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define asynclog_error(...) __async_log_print__(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define asynclog_fatal(...) __async_log_print__(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)


typedef struct {
    va_list argument_pointer;
    char *fmt;
    char *file;
    struct tm *time;
    void *ctx;
    int line;
    int level;
    char *async_message;
} LogEvent;


typedef void (*_LogCallbackFunc)(LogEvent *event);
typedef void (*_LogLockFunc)(int lock, void *ctx);


typedef struct {
    _LogCallbackFunc func;
    void *ctx;
} _LogCallback;


typedef struct _AsyncLogTask {
    char *file;
    int line;
    int level;
    struct tm time;
    char message[ASYNC_LOG_MAX_MESSAGE_LENGTH];
    struct _AsyncLogTask *next;
} _AsyncLogTask;


/**
 * @brief Initialize an asynchronous logger.
 * @param n_workers The number of threads (like `4`).
 * @param queue_capacity The maximum length of thread queue (like `32`).
**/
void async_log_init(int n_workers, int queue_capacity);


/**
 * @brief Destroy the asynchronous logger and free its memory safely.
 * @param safe_exit `1` for awaiting all tasks to be completed, `0` for exiting immediately.
**/
void async_log_exit(int safe_exit);


/**
 * @brief Set the asynchronous log mode.
 * @param mode `1` for not printing, `0` for default printing.
**/
void async_log_setting(int mode);


/**
 * @brief Configure the asynchronous logger written to the disk.
 * @param f The pointer of asynchronous log file.
**/
void async_log_config_write(FILE *f);


/**
 * @brief Configure the multi-thread lock for the asynchronous log.
 * @param func Thread lock function (go to the `async_log.h` declaration to see how to use it).
 * @param ctx A lock (e.g., a `Mutex`).
 * @example
 * @code
void mutex_func(int lock, void *ctx) {
    Mutex *mutex = (Mutex *)ctx;
    if (lock) {
        printf("-------- lock --------\n");
        mutex_lock(mutex);
    } else {
        mutex_unlock(mutex);
        printf("------- unlock -------\n");
    }
}
async_log_config_thread_lock(mutex_func, &mutex);
 * @endcode
**/
void async_log_config_thread_lock(_LogLockFunc func, void *ctx);


/**
 * @brief Add a custom callback function.
 * @param func Callback function (go to the `async_log.h` declaration to see how to use it).
 * @param ctx Store the user's context data and return it directly after the callback is executed.
 * @example
 * @code
void custom_callback(LogEvent *event) {
    printf("I want to write to a CSV file.\n");
    fprintf(event->ctx, "This, is, me, %s\n", "Illusionna");
    vfprintf(event->ctx, event->fmt, event->argument_pointer);
}
FILE *fp = fopen("demo.csv", "w");
async_log_add_callback(custom_callback, fp);
 * @endcode
**/
void async_log_add_callback(_LogCallbackFunc func, void *ctx);


/**
 * @brief Print an asynchronous log to terminal.
 * @param level The level of asynchronous log (`LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`, `LOG_FATAL`).
 * @param file The C language file.
 * @param line The C language file line.
 * @param fmt The content string.
**/
void __async_log_print__(int level, char *file, int line, char *fmt, ...);


/**
 * @brief The thread worker of asynchronous logger.
 * @param args The arguments of thread log task function.
**/
void __async_log_worker__(void *args);


/**
 * @brief Get an ownership of task object from the thread pool.
 * @return An asynchronous log task from thread pool.
**/
_AsyncLogTask *__async_log_threadpool_allocate__();


/**
 * @brief Return the ownership of task object to the thread pool.
 * @param task The asynchronous log task.
**/
void __async_log_threadpool_deallocate__(_AsyncLogTask *task);


/**
 * @brief Initialize the asynchronous log event.
 * @param event The event of asynchronous log.
 * @param ctx Store the user's context data and return it directly after the callback is executed.
**/
void __async_log_init_event__(LogEvent *event, void *ctx);


/**
 * @brief Asynchronous log enables thread lock.
**/
void __async_log_lock__();


/**
 * @brief Asynchronous log disabling thread lock.
**/
void __async_log_unlock__();


/**
 * @brief Asynchronous universal callback format.
 * @param event The event of asynchronous log.
 * @param enable_colour `1` for activation, `0` for deactivation.
**/
void __async_log_callback_common_format__(LogEvent *event, int enable_colour);


/**
 * @brief Standard output callback function.
 * @param event The event of asynchronous log.
**/
void __async_log_callback_stdout__(LogEvent *event);


/**
 * @brief Write to the disk file callback function.
 * @param event The event of asynchronous log.
**/
void __async_log_callback_write__(LogEvent *event);


#endif