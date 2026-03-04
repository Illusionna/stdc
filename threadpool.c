#include "threadpool.h"


ThreadPool *threadpool_create(int n_workers, int queue_capacity) {
    if (n_workers <= 0 || queue_capacity <= 0) return NULL;

    unsigned int pool_size = sizeof(ThreadPool);
    unsigned int queue_size = queue_capacity * sizeof(ThreadTask);
    unsigned int thread_size = n_workers * sizeof(Thread);

    unsigned int queue_offset = (pool_size + 7) & ~7;
    unsigned int thread_offset = (queue_offset + queue_size + 7) & ~7;

    unsigned int total_size = thread_offset + thread_size;

    void *sequence = malloc(total_size);
    if (!sequence) return NULL;
    memset(sequence, 0, total_size);

    ThreadPool *pool = (ThreadPool *)sequence;
    pool->queue = (ThreadTask *)((char *)sequence + queue_offset);
    pool->threads = (Thread *)((char *)sequence + thread_offset);

    pool->n_workers = n_workers;
    pool->queue_capacity = queue_capacity;

    if (mutex_create(&pool->queue_lock, 1) != 0) {
        free(pool);
        return NULL;
    }

    if (condition_init(&pool->all_idle) != 0) {
        mutex_destroy(&pool->queue_lock);
        free(pool);
        return NULL;
    }

    if (condition_init(&pool->notify) != 0) {
        condition_destroy(&pool->all_idle);
        mutex_destroy(&pool->queue_lock);
        free(pool);
        return NULL;
    }

    if (condition_init(&pool->not_full) != 0) {
        condition_destroy(&pool->notify);
        condition_destroy(&pool->all_idle);
        mutex_destroy(&pool->queue_lock);
        free(pool);
        return NULL;
    }

    for (int i = 0; i < n_workers; i++) {
        if (thread_create(&(pool->threads[i]), __threadpool_worker__, (void *)pool) != 0) {
            mutex_lock(&pool->queue_lock);
            pool->shutdown = 1;
            pool->n_workers = i;
            condition_broadcast(&pool->notify);
            mutex_unlock(&pool->queue_lock);

            for (int j = 0; j < i; j++) thread_join(&(pool->threads[j]), NULL);

            condition_destroy(&pool->not_full);
            condition_destroy(&pool->notify);
            condition_destroy(&pool->all_idle);
            mutex_destroy(&pool->queue_lock);
            free(pool);
            return NULL;
        }
    }
    return pool;
}


int threadpool_add(ThreadPool *pool, void (*func)(void *args), void *args, int block, void (*cleanup)(void *args)) {
    if (pool == NULL || func == NULL) return 1;

    mutex_lock(&pool->queue_lock);

    while (pool->queue_length == pool->queue_capacity && !pool->shutdown) {
        if (!block) {
            mutex_unlock(&pool->queue_lock);
            return 2;
        }
        condition_wait(&pool->not_full, &pool->queue_lock);
    }

    if (pool->shutdown) {
        mutex_unlock(&pool->queue_lock);
        return 1;
    }

    pool->queue[pool->queue_tail].func = func;
    pool->queue[pool->queue_tail].args = args;
    pool->queue[pool->queue_tail].cleanup = cleanup;

    pool->queue_tail = (pool->queue_tail + 1) % pool->queue_capacity;
    pool->queue_length++;

    condition_signal(&pool->notify);
    mutex_unlock(&pool->queue_lock);
    return 0;
}


int threadpool_wait(ThreadPool *pool) {
    if (pool == NULL) return 1;

    mutex_lock(&pool->queue_lock);
    while ((pool->queue_length > 0 || pool->n_working > 0) && !pool->shutdown) condition_wait(&pool->all_idle, &pool->queue_lock);
    mutex_unlock(&pool->queue_lock);
    return 0;
}


int threadpool_destroy(ThreadPool *pool, int safe_exit) {
    if (pool == NULL) return 1;

    mutex_lock(&pool->queue_lock);
    if (pool->shutdown) {
        mutex_unlock(&pool->queue_lock);
        return 1;
    }
    pool->shutdown = 1;

    if (!safe_exit) {
        for (int i = 0; i < pool->queue_length; i++) {
            int idx = (pool->queue_head + i) % pool->queue_capacity;

            ThreadTask *task = &pool->queue[idx];
            if (task->cleanup) task->cleanup(task->args);
        }
        pool->queue_length = 0;
        pool->queue_head = 0;
        pool->queue_tail = 0;
    }

    condition_broadcast(&pool->notify);
    condition_broadcast(&pool->not_full);
    mutex_unlock(&pool->queue_lock);

    for (int i = 0; i < pool->n_workers; i++) thread_join(&(pool->threads[i]), NULL);

    condition_destroy(&pool->all_idle);
    condition_destroy(&pool->notify);
    condition_destroy(&pool->not_full);
    mutex_destroy(&pool->queue_lock);
    free(pool);
    return 0;
}


int __threadpool_worker__(void *args) {
    ThreadPool *pool = (ThreadPool *)args;

    while (1) {
        mutex_lock(&pool->queue_lock);

        while (pool->queue_length == 0 && !pool->shutdown) condition_wait(&pool->notify, &pool->queue_lock);

        if (pool->shutdown && pool->queue_length == 0) {
            mutex_unlock(&pool->queue_lock);
            thread_exit();
        }

        ThreadTask task = pool->queue[pool->queue_head];

        pool->queue_head = (pool->queue_head + 1) % pool->queue_capacity;
        pool->queue_length--;

        condition_signal(&pool->not_full);
        pool->n_working++;
        mutex_unlock(&pool->queue_lock);

        if (task.func) task.func(task.args);
        if (task.cleanup) task.cleanup(task.args);

        mutex_lock(&pool->queue_lock);
        pool->n_working--;
        if (pool->n_working == 0 && pool->queue_length == 0) condition_broadcast(&pool->all_idle);
        mutex_unlock(&pool->queue_lock);
    }
    return 0;
}
