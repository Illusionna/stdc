#ifndef _MEMORYPOOL_H_
#define _MEMORYPOOL_H_


#include "type.h"
#include "thread.h"


#define MEMORYPOOL_MANAGER_SIZE 8
#define MEMORYPOOL_FALLBACK_MALLOC_INDEX 0xff


typedef struct _MemoryBlock {
    struct _MemoryBlock *next;
} _MemoryBlock;


typedef struct _MemoryPoolMeta {
    usize block_size;
    usize block_per_chunk;
    void *chunk_list;
    _MemoryBlock *free_list;
    Mutex lock;
} _MemoryPoolMeta;


typedef union {
    usize pool_idx;
    unsigned char pad[16];
} _MemoryAllocHeader;


typedef union _MemoryChunkHeader {
    union _MemoryChunkHeader *next;
    unsigned char pad[16];
} _MemoryChunkHeader;


typedef struct MemoryPool {
    _MemoryPoolMeta memory[MEMORYPOOL_MANAGER_SIZE];
} MemoryPool;


/**
 * @brief Create a memory pool.
 * @return The pointer of memory pool.
**/
MemoryPool* memorypool_create();


/**
 * @brief Destroy the memory pool and free all internally managed chunks.
 * @param pool The pointer of memory pool.
 * @warning You must ensure that all memory allocated vid `memorypool_alloc` is explicitly released using `memorypool_free` before calling this `memorypool_destroy` function.
**/
void memorypool_destroy(MemoryPool *pool);


/**
 * @brief Allocate memory in pool.
 * @param pool The pointer of memory pool.
 * @param size The size of memory.
 * @return The allocated memory address.
**/
void *memorypool_alloc(MemoryPool *pool, usize size);


/**
 * @brief Release the memory in pool.
 * @param pool The pointer of memory pool.
 * @param ptr The pointer of context in memory pool.
**/
void memorypool_free(MemoryPool *pool, void *ptr);


#endif