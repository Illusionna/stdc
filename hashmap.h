#ifndef _HASHMAP_H_
#define _HASHMAP_H_


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "type.h"


#define HASHMAP_INIT_BUCKETS 8  // The value of `HASHMAP_INIT_BUCKETS` must be the power of `2`.
#define HASHMAP_LOAD_FACTOR 0.75
#define HASHMAP_SHRINK_FACTOR 0.25


#define GenericVariable(x) _Generic((1 ? (x) : (x)), \
    char: __hashmap_var_from_char__, \
    short: __hashmap_var_from_short__, \
    int: __hashmap_var_from_int__, \
    long: __hashmap_var_from_long__, \
    long long: __hashmap_var_from_long_long__, \
    unsigned char: __hashmap_var_from_unsigned_char__, \
    unsigned short: __hashmap_var_from_unsigned_short__, \
    unsigned int: __hashmap_var_from_unsigned_int__, \
    unsigned long: __hashmap_var_from_unsigned_long__, \
    unsigned long long: __hashmap_var_from_unsigned_long_long__, \
    float: __hashmap_var_from_float__, \
    double: __hashmap_var_from_double__, \
    long double: __hashmap_var_from_long_double__, \
    signed char: __hashmap_var_from_signed_char__, \
    char *: __hashmap_var_from_string__, \
    const char *: __hashmap_var_from_const_string__, \
    void *: __hashmap_var_from_pointer__, \
    const void *: __hashmap_var_from_const_pointer__, \
    default: __hashmap_var_from_const_pointer__ \
)((x))


/**
 * @brief Put a key-value pair into the `HashMap` dictionary.
 * @param dict The `HashMap` dictionary.
 * @param key The key (not allowed to be `NULL`).
 * @param value The value (if all the element values are `NULL`, it will be decayed to a `set`).
 * @return `1` for success, `0` for failure.
**/
#define hashmap_add(dict, key, value) __hashmap_add__(dict, GenericVariable(key), GenericVariable(value))


/**
 * @brief Get the value in the `HashMap` dictionary by key.
 * @param dict The `HashMap` dictionary.
 * @param key The key.
 * @return The `HashMapVariant` pointer of value (`NULL` for failure).
**/
#define hashmap_get(dict, key) __hashmap_get__(dict, GenericVariable(key))


/**
 * @brief Remove a key-value pair from the `HashMap` dictionary.
 * @param dict The `HashMap` dictionary.
 * @param key The key.
 * @return `1` for success, `0` for failure.
**/
#define hashmap_remove(dict, key) __hashmap_remove__(dict, GenericVariable(key))


/**
 * @brief Determine whether an element exists.
 * @param dict The `HashMap` dictionary.
 * @param key The key.
 * @return `1` for existence, `0` for nonexistence.
**/
#define hashmap_contains(dict, key) __hashmap_contains__(dict, GenericVariable(key))


typedef enum {
    _HASHMAP_NULL,
    _HASHMAP_STRING,
    _HASHMAP_POINTER,
    _HASHMAP_INT32,
    _HASHMAP_INT64,
    _HASHMAP_USIZE,
    _HASHMAP_FLOAT,
    _HASHMAP_DOUBLE,
    _HASHMAP_LONGDOUBLE
} _HashMapDataType;


typedef struct HashMapVariant {
    _HashMapDataType type;
    union {
        char *str;
        void *ptr;
        int32 i;
        int64 l;
        usize u;
        float32 f;
        float64 d;
        long double ld;
    } as;
} HashMapVariant;


typedef struct _HashMapNode {
    HashMapVariant key;
    HashMapVariant value;
    struct _HashMapNode *next;
    uint32 hash;
} _HashMapNode;


typedef struct HashMap {
    _HashMapNode **table;
    uint32 buckets;
    uint64 count;
    uint32 seed;
} HashMap;


/**
 * @brief Create a new `HashMap` dictionary.
 * @return The pointer to the new `HashMap` dictionary.
**/
HashMap *hashmap_create();


/**
 * @brief Free the memory of `HashMap` dictionary.
 * @param dict The `HashMap` dictionary.
**/
void hashmap_destroy(HashMap *dict);


/**
 * @brief View the data structure of the `HashMap` dictionary.
 * @param dict The `HashMap` dictionary.
**/
void hashmap_view(HashMap *dict);


/**
 * @brief Print `HashMapVariant` type.
 * @param x The input.
**/
void hashmap_print_variant(HashMapVariant *x);


/**
 * @brief Get information about the dictionary's total memory usage.
 * @param The `HashMap` dictionary.
**/
usize __hashmap_sizeof__(HashMap *dict);


/**
 * @brief Use the FNV-1a function to calculate the hash value.
 * @param data The raw string data.
 * @param seed The random seed.
 * @return The hash value.
**/
uint32 __hash_function_fnv1a__(char *str, uint32 seed);


bool __hashmap_add__(HashMap *dict, HashMapVariant key, HashMapVariant value);


HashMapVariant *__hashmap_get__(HashMap *dict, HashMapVariant key);


bool __hashmap_remove__(HashMap *dict, HashMapVariant key);


bool __hashmap_contains__(HashMap *dict, HashMapVariant key);


uint32 __next_pow2__(uint32 n);


void __hashmap_resize__(HashMap *dict, uint32 n_buckets);


void __hashmap_variant_cleanup__(HashMapVariant x);


void __hashmap_print__(HashMapVariant x);


bool __hashmap_variant_equals__(HashMapVariant x, HashMapVariant y);


HashMapVariant __hashmap_variant_copy__(HashMapVariant x);


uint32 __hashmap_hash__(HashMapVariant x, uint32 seed);


static inline HashMapVariant __hashmap_var_from_char__(char x) {
    return (HashMapVariant){
        .type = _HASHMAP_INT32,
        .as.i = (int32)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_short__(short x) {
    return (HashMapVariant){
        .type = _HASHMAP_INT32,
        .as.i = (int32)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_int__(int x) {
    return (HashMapVariant){
        .type = _HASHMAP_INT32,
        .as.i = (int32)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_long__(long x) {
    return (HashMapVariant){
        .type = _HASHMAP_INT64,
        .as.l = (int64)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_long_long__(long long x) {
    return (HashMapVariant){
        .type = _HASHMAP_INT64,
        .as.l = (int64)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_unsigned_char__(unsigned char x) {
    return (HashMapVariant){
        .type = _HASHMAP_USIZE,
        .as.u = (usize)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_unsigned_short__(unsigned short x) {
    return (HashMapVariant){
        .type = _HASHMAP_USIZE,
        .as.u = (usize)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_unsigned_int__(unsigned int x) {
    return (HashMapVariant){
        .type = _HASHMAP_USIZE,
        .as.u = (usize)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_unsigned_long__(unsigned long x) {
    return (HashMapVariant){
        .type = _HASHMAP_USIZE,
        .as.u = (usize)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_unsigned_long_long__(unsigned long long x) {
    return (HashMapVariant){
        .type = _HASHMAP_USIZE,
        .as.u = (usize)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_float__(float x) {
    return (HashMapVariant){
        .type = _HASHMAP_FLOAT,
        .as.f = (float32)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_double__(double x) {
    return (HashMapVariant){
        .type = _HASHMAP_DOUBLE,
        .as.d = (float64)(x)
    };
}


static inline HashMapVariant __hashmap_var_from_long_double__(long double x) {
    return (HashMapVariant){
        .type = _HASHMAP_LONGDOUBLE,
        .as.ld = x
    };
}


static inline HashMapVariant __hashmap_var_from_signed_char__(signed char x) {
    return (HashMapVariant){
        .type = _HASHMAP_INT32,
        .as.i = (int32)x
    };
}


static inline HashMapVariant __hashmap_var_from_string__(char *x) {
    return (HashMapVariant){
        .type = _HASHMAP_STRING,
        .as.str = x
    };
}


static inline HashMapVariant __hashmap_var_from_const_string__(const char *x) {
    return (HashMapVariant){
        .type = _HASHMAP_STRING,
        .as.str = (char *)x
    };
}


static inline HashMapVariant __hashmap_var_from_pointer__(void *x) {
    return (HashMapVariant){
        .type = _HASHMAP_POINTER,
        .as.ptr = x
    };
}


static inline HashMapVariant __hashmap_var_from_const_pointer__(const void *x) {
    return (HashMapVariant){
        .type = _HASHMAP_POINTER,
        .as.ptr = (void *)x
    };
}


#endif
