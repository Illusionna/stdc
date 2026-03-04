#include "log.h"


struct {
    void *ctx;
    int level;
    int mode;
    _LogLockFunc lock;
    _LogCallback callback[MAX_CALLBACKS];
} _LogLock;


const char *COLOURS[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
const char *TIPS[6] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};


void log_config_write(FILE *f) {
    log_add_callback(__log_callback_write__, f);
}


void log_setting(int mode) {
    _LogLock.mode = mode;
}


void log_add_callback(_LogCallbackFunc func, void *ctx) {
    for (int n = 0; n < MAX_CALLBACKS; n++) {
        if (!_LogLock.callback[n].func) {
            _LogLock.callback[n] = (_LogCallback) {func, ctx};
            break;
        }
    }
}


void log_config_thread_lock(_LogLockFunc func, void *ctx) {
    _LogLock.lock = func;
    _LogLock.ctx = ctx;
}


void __log_print__(int level, char *file, int line, char *fmt, ...) {
    LogEvent event = {
        .fmt = fmt,
        .file = file,
        .line = line,
        .level = level
    };

    __log_lock__();

    if (!_LogLock.mode) {
        __log_init_event__(&event, stderr);
        va_start(event.argument_pointer, fmt);
        __log_callback_stdout__(&event);
        va_end(event.argument_pointer);
    }

    for (int n = 0; n < MAX_CALLBACKS && _LogLock.callback[n].func; n++) {
        _LogCallback *callback = &_LogLock.callback[n];
        __log_init_event__(&event, callback->ctx);
        va_start(event.argument_pointer, fmt);
        callback->func(&event);
        va_end(event.argument_pointer);
    }

    __log_unlock__();
}


void __log_init_event__(LogEvent *event, void *ctx) {
    if (!event->time) {
        time_t now = time(NULL);
        event->time = localtime(&now);
    }
    event->ctx = ctx;
}


void __log_lock__() {
    if (_LogLock.lock) _LogLock.lock(1, _LogLock.ctx);
}


void __log_unlock__() {
    if (_LogLock.lock) _LogLock.lock(0, _LogLock.ctx);
}


void __log_callback_stdout__(LogEvent *event) {
    char buffer[32];
    buffer[strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", event->time)] = '\0';
    fprintf(event->ctx, "%s %s%-7s \x1b[90m%s:%d:\x1b[0m ", buffer, COLOURS[event->level], TIPS[event->level], event->file, event->line);
    vfprintf(event->ctx, event->fmt, event->argument_pointer);
    fprintf(event->ctx, "\n");
    fflush(event->ctx);
}


void __log_callback_write__(LogEvent *event) {
    char buffer[32];
    buffer[strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", event->time)] = '\0';
    fprintf(event->ctx, "%s %-7s %s:%d: ", buffer, TIPS[event->level], event->file, event->line);
    vfprintf(event->ctx, event->fmt, event->argument_pointer);
    fprintf(event->ctx, "\n");
    fflush(event->ctx);
}
