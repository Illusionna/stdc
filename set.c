#include "set.h"


Set *set_create() {
    Set *set = (Set *)calloc(1, sizeof(Set));
    if (set == NULL) return NULL;
    set->used = 0;
    set->count = 0;
    set->capacity = 16;
    set->buckets = (_SetSlot *)calloc(set->capacity, sizeof(_SetSlot));
    if (set->buckets == NULL) {
        free(set);
        return NULL;
    }
    return set;
}


void set_destroy(Set *set) {
    for (unsigned long long i = 0; i < set->capacity; i++) {
        if (set->buckets[i].type == SET_SLOT_STRING) free(set->buckets[i].value.s);
    }
    free(set->buckets);
    free(set);
}


void set_view(Set *set) {
    if (set->count == 0) printf("{}_0\n");
    else {
        printf("{");
        int first = 1;
        for (unsigned long long i = 0; i < set->capacity; i++) {
            if (set->buckets[i].type <= SET_SLOT_REMOVE) continue;
            if (!first) printf(", ");
            first = 0;
            switch (set->buckets[i].type) {
                case SET_SLOT_INT:
                    printf("%d", set->buckets[i].value.i);
                    break;
                case SET_SLOT_LONG:
                    printf("%ld", set->buckets[i].value.l);
                    break;
                case SET_SLOT_FLOAT:
                    printf("%f", set->buckets[i].value.f);
                    break;
                case SET_SLOT_DOUBLE:
                    printf("%lf", set->buckets[i].value.d);
                    break;
                case SET_SLOT_STRING:
                    printf("\"%s\"", set->buckets[i].value.s);
                    break;
                default:
                    break;
            }
        }
        printf("}_%llu\n", set->count);
    }
}


unsigned int __hash_fnv_1a__(void *data, unsigned long long length) {
    unsigned int hash = 2166136261U;
    unsigned char *p = (unsigned char *)data;
    for (unsigned long long i = 0; i < length; i++) hash = (hash ^ p[i]) * 16777619U;
    return hash;
}


unsigned int __set_get_hash__(_SetSlotState type, void *value) {
    if (type == SET_SLOT_INT) return __hash_fnv_1a__(value, sizeof(int));
    if (type == SET_SLOT_LONG) return __hash_fnv_1a__(value, sizeof(long));
    if (type == SET_SLOT_FLOAT) {
        float f = *(float *)value;
        // NaN.
        if (f != f) return 0x7FC00000;
        // +0.0F or -0.0F.
        if (f == 0.0F) f = 0.0F;
        return __hash_fnv_1a__(&f, sizeof(float));
    }
    if (type == SET_SLOT_DOUBLE) {
        double d = *(double *)value;
        // NaN.
        if (d != d) return 0x7FC00000;
        // +0.0 or -0.0.
        if (d == 0.0) d = 0.0;
        return __hash_fnv_1a__(&d, sizeof(double));
    }
    if (type == SET_SLOT_STRING) return __hash_fnv_1a__(value, strlen((char *)value));
    return 0;
}


int __set_slot_equal__(_SetSlot *slot, _SetSlotState type, void *value) {
    if (slot->type != type) return 0;
    switch (type) {
        case SET_SLOT_INT:
            return slot->value.i == *(int *)value;
        case SET_SLOT_LONG:
            return slot->value.l == *(long *)value;
        case SET_SLOT_FLOAT: {
            float a = slot->value.f;
            float b = *(float *)value;
            // Determine the float value of NaN.
            if (a != a && b != b) return 1;
            return a == b;
        }
        case SET_SLOT_DOUBLE: {
            double a = slot->value.d;
            double b = *(double *)value;
            // Determine the double value of NaN.
            if (a != a && b != b) return 1;
            return a == b;
        }
        case SET_SLOT_STRING:
            return strcmp(slot->value.s, (char *)value) == 0;
        default:
            return 0;
    }
}


int __set_resize__(Set *set) {
    unsigned long long new_capacity = set->capacity * 2;
    _SetSlot *new_buckets = (_SetSlot *)calloc(new_capacity, sizeof(_SetSlot));
    if (new_buckets == NULL) return 1;

    _SetSlot *old_buckets = set->buckets;
    unsigned long long old_capacity = set->capacity;
    set->used = set->count;
    set->capacity = new_capacity;
    set->buckets = new_buckets;

    for (unsigned long long i = 0; i < old_capacity; i++) {
        _SetSlot *slot = &old_buckets[i];
        if (slot->type <= SET_SLOT_REMOVE) continue;
        unsigned long long idx = slot->hash & (set->capacity - 1);
        while (set->buckets[idx].type != SET_SLOT_EMPTY) idx = (idx + 1) & (set->capacity - 1);
        set->buckets[idx] = *slot;
    }
    free(old_buckets);
    return 0;
}


_SetSlot *__set_find_slot__(Set *set, _SetSlotState type, void *value, unsigned int hash) {
    unsigned long long idx = hash & (set->capacity - 1);
    _SetSlot *tombstone = NULL;
    while (1) {
        _SetSlot *slot = &set->buckets[idx];
        if (slot->type == SET_SLOT_EMPTY) return tombstone != NULL ? tombstone : slot;
        else if (slot->type == SET_SLOT_REMOVE) {
            if (tombstone == NULL) tombstone = slot;
        }
        else if (__set_slot_equal__(slot, type, value)) return slot;
        idx = (idx + 1) & (set->capacity - 1);
    }
}


void __set_put__(Set *set, _SetSlotState type, void *value) {
    if (set->used >= set->capacity * 0.75) {
        int status = __set_resize__(set);
        if (status == 1) {
            printf("\x1b[31m[Error] memory re-allocation failed and the program exited...\x1b[0m\n");
            set_destroy(set);
            exit(EXIT_FAILURE);
        }
    }
    unsigned int hash = __set_get_hash__(type, value);
    _SetSlot *slot = __set_find_slot__(set, type, value, hash);
    if (slot->type == type && __set_slot_equal__(slot, type, value)) return;
    if (slot->type == SET_SLOT_EMPTY) set->used++;
    slot->type = type;
    slot->hash = hash;
    if (type == SET_SLOT_INT) slot->value.i = *(int *)value;
    else if (type == SET_SLOT_LONG) slot->value.l = *(long *)value;
    else if (type == SET_SLOT_FLOAT) slot->value.f = *(float *)value;
    else if (type == SET_SLOT_DOUBLE) slot->value.d = *(double *)value;
    else if (type == SET_SLOT_STRING) slot->value.s = strdup((char *)value);
    set->count++;
}


void __set_shrink__(Set *set) {
    // if (set->capacity <= 16) return;
    unsigned long long new_capacity = set->capacity / 2;
    if (set->count >= new_capacity * 0.75) return;
    _SetSlot *new_buckets = (_SetSlot *)calloc(new_capacity, sizeof(_SetSlot));
    if (new_buckets == NULL) return;

    _SetSlot *old_buckets = set->buckets;
    unsigned long long old_capacity = set->capacity;
    set->used = set->count;
    set->capacity = new_capacity;
    set->buckets = new_buckets;

    for (unsigned long long i = 0; i < old_capacity; i++) {
        _SetSlot *slot = &old_buckets[i];
        if (slot->type <= SET_SLOT_REMOVE) continue;
        unsigned long long idx = slot->hash & (set->capacity - 1);
        while (set->buckets[idx].type != SET_SLOT_EMPTY) idx = (idx + 1) & (set->capacity - 1);
        set->buckets[idx] = *slot;
    }
    free(old_buckets);
}


void __set_del__(Set *set, _SetSlotState type, void *value) {
    unsigned int hash = __set_get_hash__(type, value);
    _SetSlot *slot = __set_find_slot__(set, type, value, hash);
    if (slot->type > SET_SLOT_REMOVE && __set_slot_equal__(slot, type, value)) {
        if (slot->type == SET_SLOT_STRING) free(slot->value.s);
        slot->type = SET_SLOT_REMOVE;
        set->count--;
        if (set->count <= set->capacity * 0.25) __set_shrink__(set);
    }
}


void __set_add_int__(Set *set, int value) {
    __set_put__(set, SET_SLOT_INT, &value);
}


void __set_del_int__(Set *set, int value) {
    __set_del__(set, SET_SLOT_INT, &value);
}


void __set_add_long__(Set *set, long value) {
    __set_put__(set, SET_SLOT_LONG, &value);
}


void __set_del_long__(Set *set, long value) {
    __set_del__(set, SET_SLOT_LONG, &value);
}


void __set_add_float__(Set *set, float value) {
    __set_put__(set, SET_SLOT_FLOAT, &value);
}


void __set_del_float__(Set *set, float value) {
    __set_del__(set, SET_SLOT_FLOAT, &value);
}


void __set_add_double__(Set *set, double value) {
    __set_put__(set, SET_SLOT_DOUBLE, &value);
}


void __set_del_double__(Set *set, double value) {
    __set_del__(set, SET_SLOT_DOUBLE, &value);
}


void __set_add_string__(Set *set, char *value) {
    __set_put__(set, SET_SLOT_STRING, value);
}


void __set_del_string__(Set *set, char *value) {
    __set_del__(set, SET_SLOT_STRING, value);
}


int __set_contains__(Set *set, _SetSlotState type, void *value) {
    unsigned int hash = __set_get_hash__(type, value);
    _SetSlot *slot = __set_find_slot__(set, type, value, hash);
    return slot->type > SET_SLOT_REMOVE && __set_slot_equal__(slot, type, value);
}


int __set_contains_int__(Set *set, int value) {
    return __set_contains__(set, SET_SLOT_INT, &value);
}


int __set_contains_long__(Set *set, long value) {
    return __set_contains__(set, SET_SLOT_LONG, &value);
}


int __set_contains_float__(Set *set, float value) {
    return __set_contains__(set, SET_SLOT_FLOAT, &value);
}


int __set_contains_double__(Set *set, double value) {
    return __set_contains__(set, SET_SLOT_DOUBLE, &value);
}


int __set_contains_string__(Set *set, char *value) {
    return __set_contains__(set, SET_SLOT_STRING, value);
}