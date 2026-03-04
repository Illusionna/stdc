#include "thread.h"


#if defined(__OS_UNIX__)
    void *__thread_wrapper__(void *args) {
        _ThreadInformation *info = (_ThreadInformation *)args;
        _ThreadFunction f = info->ptr;
        void *x = info->args;
        free(info);
        void *result = malloc(sizeof(int));
        if (result != NULL) *(int *)result = f(x);
        return result;
    }
#elif defined(__OS_WINDOWS__)
    unsigned WINAPI __thread_wrapper__(void *args) {
        _ThreadInformation *info = (_ThreadInformation *)args;
        _ThreadFunction f = info->ptr;
        void *x = info->args;
        free(info);
        return f(x);
    }
#endif


int thread_create(Thread *thread, _ThreadFunction func, void *args) {
    _ThreadInformation *info = malloc(sizeof(_ThreadInformation));
    if (info == NULL) return 1;

    info->ptr = func;
    info->args = args;

    #if defined(__OS_UNIX__)
        if (pthread_create(thread, NULL, __thread_wrapper__, (void *)info) != 0) *thread = 0;
    #elif defined(__OS_WINDOWS__)
        *thread = (HANDLE)_beginthreadex(NULL, 0, __thread_wrapper__, (void *)info, 0, NULL);
    #endif

    if (!*thread) {
        free(info);
        return 1;
    }
    return 0;
}


int thread_join(Thread *thread, int *result) {
    #if defined(__OS_UNIX__)
        void *u;
        int ans = 0;
        if (pthread_join(*thread, &u) != 0) return 1;
        if (u != NULL) {
            ans = *(int *)u;
            free(u);
        }
        if (result != NULL) *result = ans;
    #elif defined(__OS_WINDOWS__)
        if (WaitForSingleObject(*thread, 0xffffffff) == (DWORD)0xffffffff) return 1;
        if (result != NULL) {
            DWORD d;
            GetExitCodeThread(*thread, &d);
            *result = d;
        }
    #endif
    return 0;
}


int thread_detach(Thread *thread) {
    #if defined(__OS_UNIX__)
        return pthread_detach(*thread) == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        return CloseHandle(*thread) != 0 ? 0 : 1;
    #endif
}


void thread_exit() {
    #if defined(__OS_UNIX__)
        pthread_exit(NULL);
    #elif defined(__OS_WINDOWS__)
        _endthreadex(0);
    #endif
}


int mutex_create(Mutex *mutex, int type) {
    #if defined(__OS_UNIX__)
        pthread_mutexattr_t t;
        pthread_mutexattr_init(&t);
        if (type & 8) pthread_mutexattr_settype(&t, PTHREAD_MUTEX_RECURSIVE);
        int res = pthread_mutex_init(mutex, &t);
        pthread_mutexattr_destroy(&t);
        return res == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        mutex->status = 0;
        mutex->recursive = type & 8;
        InitializeCriticalSection(&mutex->cs);
        return 0;
    #endif
}


void mutex_destroy(Mutex *mutex) {
    #if defined(__OS_UNIX__)
        pthread_mutex_destroy(mutex);
    #elif defined(__OS_WINDOWS__)
        DeleteCriticalSection(&mutex->cs);
    #endif
}


int mutex_lock(Mutex *mutex) {
    #if defined(__OS_UNIX__)
        return pthread_mutex_lock(mutex) == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        EnterCriticalSection(&mutex->cs);
        if (!mutex->recursive) {
            while (mutex->status) Sleep(1000);  // Simulate deadlock.
            mutex->status = 1;
        }
        return 0;
    #endif
}


int mutex_trylock(Mutex *mutex) {
    #if defined(__OS_UNIX__)
        return pthread_mutex_trylock(mutex) == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        int res = TryEnterCriticalSection(&mutex->cs) ? 0 : 1;
        if ((!mutex->recursive) && (res == 1) && mutex->status) {
            LeaveCriticalSection(&mutex->cs);
            res = 1;
        }
        return res;
    #endif
}


int mutex_unlock(Mutex *mutex) {
    #if defined(__OS_UNIX__)
        return pthread_mutex_unlock(mutex) == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        mutex->status = 0;
        LeaveCriticalSection(&mutex->cs);
        return 0;
    #endif
}


int condition_init(ThreadCondition *condition) {
    #if defined(__OS_UNIX__)
        return pthread_cond_init(condition, NULL) == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        condition->waiter_count = 0;
        InitializeCriticalSection(&condition->cs);
        condition->events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (condition->events[0] == NULL) {
            condition->events[1] = NULL;
            return 1;
        }
        condition->events[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (condition->events[1] == NULL) {
            CloseHandle(condition->events[0]);
            condition->events[0] = NULL;
            return 1;
        }
        return 0;
    #endif
}


void condition_destroy(ThreadCondition *condition) {
    #if defined(__OS_UNIX__)
        pthread_cond_destroy(condition);
    #elif defined(__OS_WINDOWS__)
        if (condition->events[0] != NULL) CloseHandle(condition->events[0]);
        if (condition->events[1] != NULL) CloseHandle(condition->events[1]);
        DeleteCriticalSection(&condition->cs);
    #endif
}


int condition_wait(ThreadCondition *condition, Mutex *mutex) {
    #if defined(__OS_UNIX__)
        return pthread_cond_wait(condition, mutex) == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        return __condition_timedwait_win32__(condition, mutex, INFINITE);
    #endif
}


int condition_signal(ThreadCondition *condition) {
    #if defined(__OS_UNIX__)
        return pthread_cond_signal(condition) == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        EnterCriticalSection(&condition->cs);
        int have_waiters = (condition->waiter_count > 0);
        LeaveCriticalSection(&condition->cs);
        if (have_waiters) if (SetEvent(condition->events[0]) == 0) return 1;
        return 0;
    #endif
}


int condition_broadcast(ThreadCondition *condition) {
    #if defined(__OS_UNIX__)
        return pthread_cond_broadcast(condition) == 0 ? 0 : 1;
    #elif defined(__OS_WINDOWS__)
        EnterCriticalSection(&condition->cs);
        int have_waiters = (condition->waiter_count > 0);
        LeaveCriticalSection(&condition->cs);
        if (have_waiters) if (SetEvent(condition->events[1]) == 0) return 1;
        return 0;
    #endif
}


#if defined(__OS_WINDOWS__)
    int __condition_timedwait_win32__(ThreadCondition *condition, Mutex *mutex, DWORD timeout) {
        EnterCriticalSection(&condition->cs);
        ++condition->waiter_count;
        LeaveCriticalSection(&condition->cs);
        mutex_unlock(mutex);
        DWORD result = WaitForMultipleObjects(2, condition->events, FALSE, timeout);
        if (result == WAIT_TIMEOUT) {
            mutex_lock(mutex);
            return 1;
        }
        if (result == WAIT_FAILED) {
            mutex_lock(mutex);
            return 1;
        }
        EnterCriticalSection(&condition->cs);
        --condition->waiter_count;
        int last_waiter = (result == (WAIT_OBJECT_0 + 1)) && (condition->waiter_count == 0);
        LeaveCriticalSection(&condition->cs);
        if (last_waiter) {
            if (ResetEvent(condition->events[1]) == 0) {
                mutex_lock(mutex);
                return 1;
            }
        }
        mutex_lock(mutex);
        return 0;
    }
#endif