#ifndef _SET_H_
#define _SET_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define set_add(set, value) do { \
    _Generic((value), \
        int: __set_add_int__, \
        long: __set_add_long__, \
        float: __set_add_float__, \
        double: __set_add_double__, \
        char *: __set_add_string__ \
    )(set, value); \
} while (0)


#define set_remove(set, value) do { \
    _Generic((value), \
        int: __set_del_int__, \
        long: __set_del_long__, \
        float: __set_del_float__, \
        double: __set_del_double__, \
        char *: __set_del_string__ \
    )(set, value); \
} while (0)


/**
 * @brief Determine whether an element exists.
 * @param set The pointer of set.
 * @param value The value of data.
 * @return `1` for existence, `0` for nonexistence.
**/
#define set_contains(set, value) _Generic((value), \
    int: __set_contains_int__, \
    long: __set_contains_long__, \
    float: __set_contains_float__, \
    double: __set_contains_double__, \
    char *: __set_contains_string__ \
)(set, value)


typedef enum {
    SET_SLOT_EMPTY,
    SET_SLOT_REMOVE,
    SET_SLOT_INT,
    SET_SLOT_LONG,
    SET_SLOT_FLOAT,
    SET_SLOT_DOUBLE,
    SET_SLOT_STRING
} _SetSlotState;


typedef struct {
    _SetSlotState type;
    unsigned int hash;
    union {
        int i;
        long l;
        float f;
        double d;
        char *s;
    } value;
} _SetSlot;


typedef struct {
    _SetSlot *buckets;
    unsigned long long capacity;
    unsigned long long count;
    unsigned long long used;
} Set;


/**
 * @brief Create a set.
 * @return The pointer of set.
**/
Set *set_create();


/**
 * @brief Free the memory of set.
 * @param set The pointer of set.
**/
void set_destroy(Set *set);


/**
 * @brief Print the view of a set.
**/
void set_view(Set *set);


/**
 * @brief Use the FNV-1a function to calculate the hash value.
 * @param data The raw data.
 * @param length The length of data.
 * @return The hash value.
**/
unsigned int __hash_fnv_1a__(void *data, unsigned long long length);


/**
 * @brief Get the hash value of data.
 * @param type The set slot state type of data.
 * @param value The data value.
 * @return The hash value.
**/
unsigned int __set_get_hash__(_SetSlotState type, void *value);


/**
 * @brief Determine whether the entity elements of two slots are equal.
 * @param slot The entity element.
 * @param type The type of entity element.
 * @param value The data value.
 * @return `1` for success (equal), `0` for failure (unequal).
**/
int __set_slot_equal__(_SetSlot *slot, _SetSlotState type, void *value);


/**
 * @brief Expand the capacity of set.
 * @param set The pointer of set.
 * @return `0` for success, `1` for failure.
**/
int __set_resize__(Set *set);


/**
 * @brief Find the corresponding slot for the hash value.
 * @param set The pointer of set.
 * @param type The type of entity element.
 * @param value The data value.
 * @param hash The input hash value.
 * @return The pointer of set slot.
**/
_SetSlot *__set_find_slot__(Set *set, _SetSlotState type, void *value, unsigned int hash);


/**
 * @brief Put an element into the set.
 * @param set The pointer of set.
 * @param type The type of entity element.
 * @param value The data value.
**/
void __set_put__(Set *set, _SetSlotState type, void *value);


/**
 * @brief If the number of elements in the set is small, execute capacity reduction.
 * @param set The pointer of set.
**/
void __set_shrink__(Set *set);


/**
 * @brief Delete an element in the set.
 * @param set The pointer of set.
 * @param type The type of entity element.
 * @param value The data value.
**/
void __set_del__(Set *set, _SetSlotState type, void *value);


/**
 * @brief A wrapper function determining whether an element exists.
 * @param set The pointer of set.
 * @param type The type of set slot.
 * @param value The value of data.
 * @return `1` for existence, `0` for nonexistence.
**/
int __set_contains__(Set *set, _SetSlotState type, void *value);


void __set_add_int__(Set *set, int value);


void __set_del_int__(Set *set, int value);


void __set_add_long__(Set *set, long value);


void __set_del_long__(Set *set, long value);


void __set_add_float__(Set *set, float value);


void __set_del_float__(Set *set, float value);


void __set_add_double__(Set *set, double value);


void __set_del_double__(Set *set, double value);


void __set_add_string__(Set *set, char *value);


void __set_del_string__(Set *set, char *value);


int __set_contains_int__(Set *set, int value);


int __set_contains_long__(Set *set, long value);


int __set_contains_float__(Set *set, float value);


int __set_contains_double__(Set *set, double value);


int __set_contains_string__(Set *set, char *value);


#endif