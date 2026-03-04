#include "async_log.h"


const char *TIPS[6] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
const char *COLOURS[6] = {"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};


struct {
    void *ctx;
    int level;
    int mode;
    _LogLockFunc lock;
    _LogCallback callback[ASYNC_LOG_MAX_CALLBACKS];
    ThreadPool *pool;
    _AsyncLogTask array[ASYNC_LOG_MAX_THREAD_POOL_SIZE];
    _AsyncLogTask *linklist;
} _LogLock;


void async_log_init(int n_workers, int queue_capacity) {
    _LogLock.pool = threadpool_create(n_workers, queue_capacity);
    for (int i = 0; i < ASYNC_LOG_MAX_THREAD_POOL_SIZE - 1; i++) _LogLock.array[i].next = &_LogLock.array[i + 1];
    _LogLock.array[ASYNC_LOG_MAX_THREAD_POOL_SIZE - 1].next = NULL;
    _LogLock.linklist = &_LogLock.array[0];
}


void async_log_exit(int safe_exit) {
    threadpool_destroy(_LogLock.pool, safe_exit); 
}


void async_log_setting(int mode) {
    _LogLock.mode = mode;
}


void async_log_config_write(FILE *f) {
    async_log_add_callback(__async_log_callback_write__, f);
}


void async_log_config_thread_lock(_LogLockFunc func, void *ctx) {
    _LogLock.lock = func;
    _LogLock.ctx = ctx;
}


void async_log_add_callback(_LogCallbackFunc func, void *ctx) {
    for (int n = 0; n < ASYNC_LOG_MAX_CALLBACKS; n++) {
        if (!_LogLock.callback[n].func) {
            _LogLock.callback[n] = (_LogCallback) {func, ctx};
            break;
        }
    }
}


void __async_log_print__(int level, char *file, int line, char *fmt, ...) {
    if (_LogLock.pool) {
        __async_log_lock__();
        _AsyncLogTask *task = __async_log_threadpool_allocate__();
        __async_log_unlock__();

        if (!task) {
            fprintf(stderr, "Async log thread pool exhausted!\n");
            return;
        }

        task->level = level;
        task->file = file;
        task->line = line;
        time_t now = time(NULL);
        task->time = *localtime(&now);
        va_list args;
        va_start(args, fmt);
        vsnprintf(task->message, ASYNC_LOG_MAX_MESSAGE_LENGTH, fmt, args);
        va_end(args);

        if (threadpool_add(_LogLock.pool, __async_log_worker__, task, 1, NULL) != 0) {
            __async_log_lock__();
            __async_log_threadpool_deallocate__(task);
            __async_log_unlock__();
        }
        return;
    }

    LogEvent event = {
        .fmt = fmt,
        .file = file,
        .line = line,
        .level = level,
        .async_message = NULL
    };

    __async_log_lock__();
    if (!_LogLock.mode) {
        __async_log_init_event__(&event, stderr);
        va_start(event.argument_pointer, fmt);
        __async_log_callback_stdout__(&event);
        va_end(event.argument_pointer);
    }
    for (int n = 0; n < ASYNC_LOG_MAX_CALLBACKS && _LogLock.callback[n].func; n++) {
        _LogCallback *callback = &_LogLock.callback[n];
        __async_log_init_event__(&event, callback->ctx);
        va_start(event.argument_pointer, fmt);
        callback->func(&event);
        va_end(event.argument_pointer);
    }
    __async_log_unlock__();
}


void __async_log_worker__(void *args) {
    _AsyncLogTask *task = (_AsyncLogTask *)args;
    LogEvent event = {
        .fmt = NULL,
        .file = task->file,
        .line = task->line,
        .level = task->level,
        .time = &task->time,
        .async_message = task->message
    };

    __async_log_lock__();
    if (!_LogLock.mode) {
        __async_log_init_event__(&event, stderr);
        __async_log_callback_stdout__(&event);
    }
    for (int n = 0; n < ASYNC_LOG_MAX_CALLBACKS && _LogLock.callback[n].func; n++) {
        _LogCallback *callback = &_LogLock.callback[n];
        __async_log_init_event__(&event, callback->ctx);
        callback->func(&event);
    }
    __async_log_threadpool_deallocate__(task);
    __async_log_unlock__();
}


_AsyncLogTask *__async_log_threadpool_allocate__() {
    // If the thread pool is full, discard the logger.
    if (_LogLock.linklist == NULL) return NULL;
    _AsyncLogTask *task = _LogLock.linklist;
    _LogLock.linklist = task->next;
    return task;
}


void __async_log_threadpool_deallocate__(_AsyncLogTask *task) {
    if (!task) return;
    task->next = _LogLock.linklist;
    _LogLock.linklist = task;
}


void __async_log_init_event__(LogEvent *event, void *ctx) {
    if (!event->time) {
        time_t now = time(NULL);
        event->time = localtime(&now);
    }
    event->ctx = ctx;
}


void __async_log_lock__() {
    if (_LogLock.lock) _LogLock.lock(1, _LogLock.ctx);
}


void __async_log_unlock__() {
    if (_LogLock.lock) _LogLock.lock(0, _LogLock.ctx);
}


void __async_log_callback_common_format__(LogEvent *event, int enable_colour) {
    char buffer[ASYNC_LOG_MAX_MESSAGE_LENGTH + 256];
    int offset = 0;
    int remaining = sizeof(buffer);
    char time_buffer[32];
    time_buffer[strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", event->time)] = '\0';

    int n;
    if (enable_colour == 1) n = snprintf(buffer + offset, remaining, "%s %s%-7s \x1b[90m%s:%d:\x1b[0m ", time_buffer, COLOURS[event->level], TIPS[event->level], event->file, event->line);
    else n = snprintf(buffer + offset, remaining, "%s %-7s %s:%d: ", time_buffer, TIPS[event->level], event->file, event->line);

    if (n > 0) {
        offset = offset + n;
        remaining = remaining - n;
    }

    if (event->async_message) n = snprintf(buffer + offset, remaining, "%s", event->async_message);
    else {
        va_list args;
        va_copy(args, event->argument_pointer);
        n = vsnprintf(buffer + offset, remaining, event->fmt, args);
        va_end(args);
    }

    if (n > 0) {
        offset = offset + n;
        remaining = remaining - n;
    }
    if (remaining > 1) {
        buffer[offset++] = '\n';
        buffer[offset] = '\0';
    }

    FILE *out = (FILE *)event->ctx;
    // Use `flockfile()` to further ensure atomicity on stderr.
    os_flockfile(out);
    fwrite(buffer, 1, offset, out);
    os_funlockfile(out);
    if (event->level >= 4) fflush(out);
}


void __async_log_callback_write__(LogEvent *event) {
    __async_log_callback_common_format__(event, 0);
}


void __async_log_callback_stdout__(LogEvent *event) {
    __async_log_callback_common_format__(event, 1);
}