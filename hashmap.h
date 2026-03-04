#ifndef _HASHMAP_H_
#define _HASHMAP_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Node {
    char *key;
    char *value;
    struct Node *next;
} Node;


typedef struct {
    Node **table;
    int bucket;
    int count;
} HashMap;


/**
 * @brief ELF function in Unix HashMap.
 * @param str The dictionary key.
 * @param size The HashMap bucket.
 * @return The hash value.
**/
unsigned int __hash_function_ELF__(char *str, int size);


/**
 * @brief Expand the HashMap when the load factor exceeds 75%.
 * @param dict The HashMap dictionary.
**/
void __hashmap_expand__(HashMap *dict);


/**
 * @brief Shrink the HashMap when the load factor is below 25%.
 * @param dict The HashMap dictionary.
**/
void __hashmap_shrink__(HashMap *dict);


/**
 * @brief Create a new HashMap dictionary.
 * @return The pointer to the new HashMap dictionary.
**/
HashMap *hashmap_create();


/**
 * @brief Insert a key-value pair into the HashMap dictionary.
 * @param dict The HashMap dictionary.
 * @param key The string key.
 * @param value The string value.
**/
void hashmap_put(HashMap *dict, char *key, char *value);


/**
 * @brief Get the value in the HashMap dictionary by key.
 * @param dict The HashMap dictionary.
 * @param key The string key.
 * @return The string value.
**/
char *hashmap_get(HashMap *dict, char *key);


/**
 * @brief Remove a key-value pair from the HashMap dictionary.
 * @param dict The HashMap dictionary.
 * @param key The string key.
 * @return 0 --> success; 1 --> failed.
**/
int hashmap_remove(HashMap *dict, char *key);


/**
 * @brief Free the memory of HashMap dictionary.
 * @param dict The HashMap dictionary.
**/
void hashmap_destroy(HashMap *dict);


/**
 * @brief View the data structure of the HashMap dictionary.
 * @param dict The HashMap dictionary.
**/
void hashmap_view(HashMap *dict);


/**
 * @brief Print the key-value pairs(items) in the HashMap dictionary.
 * @param dict The HashMap dictionary.
 * @return The return string.
**/
// char *hashmap_print(HashMap *dict);


#endif
