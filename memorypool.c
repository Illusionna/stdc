#include "memorypool.h"


const usize MEMORYPOOL_MANAGER[MEMORYPOOL_MANAGER_SIZE] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
const usize MEMORYPOOL_BLOCKS[MEMORYPOOL_MANAGER_SIZE] = {1024, 512, 256, 128, 64, 32, 16, 8};


MemoryPool *memorypool_create() {
    MemoryPool *pool = malloc(sizeof(MemoryPool));
    if (!pool) return NULL;

    for (int i = 0; i < MEMORYPOOL_MANAGER_SIZE; i++) {
        pool->memory[i].block_size = MEMORYPOOL_MANAGER[i] + sizeof(_MemoryAllocHeader);
        pool->memory[i].block_per_chunk = MEMORYPOOL_BLOCKS[i];
        pool->memory[i].free_list = NULL;
        pool->memory[i].chunk_list = NULL;
        mutex_create(&pool->memory[i].lock, 1);
    }
    return pool;
}


void memorypool_destroy(MemoryPool *pool) {
    if (!pool) return;

    for (int i = 0; i < MEMORYPOOL_MANAGER_SIZE; i++) {
        _MemoryPoolMeta *p = &pool->memory[i];
        _MemoryChunkHeader *chunk = (_MemoryChunkHeader *)p->chunk_list;
        while (chunk) {
            _MemoryChunkHeader *next = chunk->next;
            free(chunk);
            chunk = next;
        }
        mutex_destroy(&p->lock);
    }
    free(pool);
}


void *memorypool_alloc(MemoryPool *pool, usize size) {
    if (!pool || size == 0) return NULL;

    _MemoryPoolMeta *p = NULL;
    usize idx = MEMORYPOOL_FALLBACK_MALLOC_INDEX;

    for (int i = 0; i < MEMORYPOOL_MANAGER_SIZE; i++) {
        if (MEMORYPOOL_MANAGER[i] >= size) {
            p = &pool->memory[i];
            idx = i;
            break;
        }
    }

    if (!p) {
        _MemoryAllocHeader *allocator = malloc(sizeof(_MemoryAllocHeader) + size);
        if (!allocator) return NULL;
        allocator->pool_idx = MEMORYPOOL_FALLBACK_MALLOC_INDEX;
        return (void *)(allocator + 1);
    }

    mutex_lock(&p->lock);
    if (!p->free_list) {
        usize chunk_bytes = sizeof(_MemoryChunkHeader) + p->block_per_chunk * p->block_size;
        _MemoryChunkHeader *chunk = malloc(chunk_bytes);
        if (!chunk) {
            mutex_unlock(&p->lock);
            return NULL;
        }

        chunk->next = (_MemoryChunkHeader *)p->chunk_list;
        p->chunk_list = chunk;

        char *start = (char *)(chunk + 1);
        for (usize i = 0; i < p->block_per_chunk; i++) {
            _MemoryBlock *block = (_MemoryBlock *)(start + i * p->block_size);
            block->next = p->free_list;
            p->free_list = block;
        }
    }
    _MemoryBlock *block = p->free_list;
    p->free_list = block->next;
    mutex_unlock(&p->lock);

    _MemoryAllocHeader *allocator = (_MemoryAllocHeader *)block;
    allocator->pool_idx = idx;
    return (void *)(allocator + 1);
}


void memorypool_free(MemoryPool *pool, void *ptr) {
    if (!pool || !ptr) return;

    _MemoryAllocHeader *allocator = ((_MemoryAllocHeader *)ptr) - 1;
    usize idx = allocator->pool_idx;
    if (idx == MEMORYPOOL_FALLBACK_MALLOC_INDEX) {
        free(allocator);
        return;
    }

    if (idx >= MEMORYPOOL_MANAGER_SIZE) return;

    _MemoryPoolMeta *p = &pool->memory[idx];
    mutex_lock(&p->lock);
    _MemoryBlock *block = (_MemoryBlock *)allocator;
    block->next = p->free_list;
    p->free_list = block;
    mutex_unlock(&p->lock);
}
