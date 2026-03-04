#include "hashmap.h"


HashMap *hashmap_create() {
    HashMap *dict = malloc(sizeof(HashMap));
    if (!dict) return NULL;

    dict->table = calloc(HASHMAP_INIT_BUCKETS, sizeof(_HashMapNode *));
    if (!dict->table) {
        free(dict);
        return NULL;
    }

    dict->buckets = HASHMAP_INIT_BUCKETS;
    dict->count = 0;
    dict->seed = (uint32)time(NULL) ^ (uint32)(uintptr)dict;
    return dict;
}


void hashmap_destroy(HashMap *dict) {
    if (!dict) return;

    for (uint32 i = 0; i < dict->buckets; i++) {
        _HashMapNode *node = dict->table[i];
        while (node) {
            _HashMapNode *temp = node;
            node = node->next;
            __hashmap_variant_cleanup__(temp->key);
            __hashmap_variant_cleanup__(temp->value);
            free(temp);
        }
    }

    free(dict->table);
    free(dict);
}


void hashmap_print_variant(HashMapVariant *x) {
    if (x == NULL) {
        printf("\x1b[1;31mnull\x1b[0m\n");
        return;
    }
    __hashmap_print__(*x);
    printf("\n");
}


void hashmap_view(HashMap *dict) {
    if (!dict) return;

    int max_bucket_index = dict->buckets > 0 ? dict->buckets - 1 : 0;
    int width = snprintf(NULL, 0, "%d", max_bucket_index);

    for (uint32 i = 0; i < dict->buckets; i++) {
        printf("bucket[\x1b[1;37m%*u\x1b[0m] = ", width, i);
        _HashMapNode *node = dict->table[i];
        if (!node) {
            printf("(null)\n");
            continue;
        }
        while (node) {
            printf("{");
            __hashmap_print__(node->key);
            printf(": ");
            __hashmap_print__(node->value);
            printf("} -> ");
            node = node->next;
        }
        printf("(null)\n");
    }

    char buffer[32];
    time_t now;
    time(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localtime(&now));

    printf("---------------- %s ----------------\n", buffer);
    printf("Buckets = \x1b[1;37m%u\x1b[0m | Count = \x1b[1;37m%llu\x1b[0m | Load = \x1b[1;37m%.3f%%\x1b[0m | Memory Usage\n", dict->buckets, dict->count, 100.0F * dict->count / dict->buckets);

    usize total = __hashmap_sizeof__(dict);
    usize struct_size = sizeof(HashMap);
    usize table_size = (usize)dict->buckets * sizeof(_HashMapNode *);
    usize nodes_size = (usize)dict->count * sizeof(_HashMapNode);
    usize strings_size = total - struct_size - table_size - nodes_size;

    printf("HashMap Struct = \x1b[1;37m%zu B\x1b[0m\n", struct_size);
    printf("Bucket Table = \x1b[1;37m%zu B\x1b[0m (%u buckets x %zu B)\n", table_size, dict->buckets, sizeof(_HashMapNode *));
    printf("Node = \x1b[1;37m%zu B\x1b[0m (%llu nodes x %zu B)\n", nodes_size, dict->count, sizeof(_HashMapNode));
    printf("String Heap = \x1b[1;37m%zu B\x1b[0m\n", strings_size);
    printf("Total = \x1b[1;37m%zu B\x1b[0m (\x1b[1;37m%.3f KB\x1b[0m)\n", total, (float)total / 1024.0F);
}


bool __hashmap_add__(HashMap *dict, HashMapVariant key, HashMapVariant value) {
    if (!dict || key.type == _HASHMAP_NULL) return False;
    if (key.type == _HASHMAP_STRING && !key.as.str) return False;
    if (key.type == _HASHMAP_POINTER && !key.as.ptr) return False;

    uint32 hash = __hashmap_hash__(key, dict->seed);
    uint32 index = hash & (dict->buckets - 1);

    for (_HashMapNode *node = dict->table[index]; node; node = node->next) {
        if (__hashmap_variant_equals__(node->key, key)) {
            HashMapVariant var = __hashmap_variant_copy__(value);
            if (value.type == _HASHMAP_STRING && value.as.str && var.type == _HASHMAP_NULL) return False;
            __hashmap_variant_cleanup__(node->value);
            node->value = var;
            return True;
        }
    }

    if (dict->count >= dict->buckets * HASHMAP_LOAD_FACTOR) __hashmap_resize__(dict, 2 * dict->buckets);

    HashMapVariant key_copy = __hashmap_variant_copy__(key);
    if (key.type == _HASHMAP_STRING && key.as.str && key_copy.type == _HASHMAP_NULL) return False;

    HashMapVariant value_copy = __hashmap_variant_copy__(value);
    if (value.type == _HASHMAP_STRING && value.as.str && value_copy.type == _HASHMAP_NULL) {
        __hashmap_variant_cleanup__(key_copy);
        return False;
    }

    _HashMapNode *node = malloc(sizeof(_HashMapNode));
    if (!node) {
        __hashmap_variant_cleanup__(key_copy);
        __hashmap_variant_cleanup__(value_copy);
        return False;
    }

    index = hash & (dict->buckets - 1);
    node->hash = hash;
    node->key = key_copy;
    node->value = value_copy;
    node->next = dict->table[index];
    dict->table[index] = node;
    dict->count++;
    return True;
}


uint32 __hash_function_fnv1a__(char *str, uint32 seed) {
    uint32 hash = 2166136261U ^ seed;
    while (*str) hash = (hash ^ (unsigned char)(*str++)) * 16777619U;
    return hash;
}


uint32 __hashmap_hash__(HashMapVariant x, uint32 seed) {
    uint32 hash = 0;
    switch (x.type) {
        case _HASHMAP_STRING:
            return __hash_function_fnv1a__(x.as.str, seed);
        case _HASHMAP_POINTER: {
            uint64 p = (uint64)(uintptr)x.as.ptr;
            return (uint32)(((p ^ seed) ^ (p >> 32)) * 2654435761U);
        }
        case _HASHMAP_INT32:
            return ((x.as.i ^ seed) * 2654435761U);
        case _HASHMAP_INT64:
            return (uint32)((((uint64)x.as.l ^ seed) * 11400714819323198485ULL) >> 32);
        case _HASHMAP_USIZE:
            return ((x.as.u ^ seed) * 2654435761U);
        case _HASHMAP_FLOAT: {
            if (x.as.f != x.as.f) return seed ^ 0x7FC00000U;
            uint32 bits;
            if (x.as.f == 0.0F) {
                uint32 zero = 0;
                memcpy(&bits, &zero, sizeof(bits));
            } else {
                memcpy(&bits, &x.as.f, sizeof(bits));
            }
            return (bits ^ seed) * 2654435761U; // Fibonacci.
        }
        case _HASHMAP_DOUBLE: {
            if (x.as.d != x.as.d) return seed ^ 0x7FC00000U;
            uint64 bits;
            if (x.as.d == 0.0) {
                uint64 zero = 0;
                memcpy(&bits, &zero, sizeof(bits));
            } else {
                memcpy(&bits, &x.as.d, sizeof(bits));
            }
            return (uint32)(((bits ^ seed) ^ (bits >> 32)) * 2654435761U);
        }
        case _HASHMAP_LONGDOUBLE: {
            if (x.as.ld != x.as.ld) return seed ^ 0x7FC00000U;
            if (x.as.ld == 0.0L) x.as.ld = 0.0L;
            hash = seed;
            unsigned char *p = (unsigned char *)&x.as.ld;
            for (usize i = 0; i < sizeof(long double); i++) hash = (hash * 31) + p[i];
            return hash;
        }
        default:
            return 0;
    }
}


uint32 __next_pow2__(uint32 n) {
    if (n == 0) return 1;
    n--;
    n = n | (n >> 1);
    n = n | (n >> 2);
    n = n | (n >> 4);
    n = n | (n >> 8);
    n = n | (n >> 16);
    return n + 1;
}


HashMapVariant __hashmap_variant_copy__(HashMapVariant x) {
    if (x.type == _HASHMAP_STRING && x.as.str) {
        x.as.str = strdup(x.as.str);
        if (!x.as.str) {
            return (HashMapVariant){.type = _HASHMAP_NULL, .as = {0}};
        }
    }
    return x;
}


HashMapVariant *__hashmap_get__(HashMap *dict, HashMapVariant key) {
    if (!dict || key.type == _HASHMAP_NULL) return NULL;
    if (key.type == _HASHMAP_STRING && !key.as.str) return NULL;
    if (key.type == _HASHMAP_POINTER && !key.as.ptr) return NULL;

    _HashMapNode *node = dict->table[__hashmap_hash__(key, dict->seed) & (dict->buckets - 1)];
    while (node) {
        if (__hashmap_variant_equals__(node->key, key)) return &node->value;
        node = node->next;
    }
    return NULL;
}


bool __hashmap_contains__(HashMap *dict, HashMapVariant key) {
    return __hashmap_get__(dict, key) != NULL;
}


bool __hashmap_remove__(HashMap *dict, HashMapVariant key) {
    if (!dict || dict->count == 0 || key.type == _HASHMAP_NULL) return False;
    if (key.type == _HASHMAP_STRING && !key.as.str) return False;
    if (key.type == _HASHMAP_POINTER && !key.as.ptr) return False;

    uint32 hash = __hashmap_hash__(key, dict->seed);
    uint32 index = hash & (dict->buckets - 1);
    _HashMapNode *node = dict->table[index];
    _HashMapNode *prev = NULL;

    while (node) {
        if (__hashmap_variant_equals__(node->key, key)) {
            if (prev) prev->next = node->next;
            else dict->table[index] = node->next;

            __hashmap_variant_cleanup__(node->key);
            __hashmap_variant_cleanup__(node->value);
            free(node);
            dict->count--;

            if (dict->buckets > HASHMAP_INIT_BUCKETS && dict->count <= HASHMAP_SHRINK_FACTOR * dict->buckets) __hashmap_resize__(dict, dict->buckets / 2);
            return True;
        }
        prev = node;
        node = node->next;
    }
    return False;
}


bool __hashmap_variant_equals__(HashMapVariant x, HashMapVariant y) {
    if (x.type != y.type) return False;
    switch (x.type) {
        case _HASHMAP_STRING:
            if (!x.as.str || !y.as.str) return x.as.str == y.as.str;
            return strcmp(x.as.str, y.as.str) == 0;
        case _HASHMAP_POINTER:
            return x.as.ptr == y.as.ptr;
        case _HASHMAP_INT32:
            return x.as.i == y.as.i;
        case _HASHMAP_INT64:
            return x.as.l == y.as.l;
        case _HASHMAP_USIZE:
            return x.as.u == y.as.u;
        case _HASHMAP_FLOAT:
            if (x.as.f != x.as.f && y.as.f != y.as.f) return True;
            return x.as.f == y.as.f;
        case _HASHMAP_DOUBLE:
            if (x.as.d != x.as.d && y.as.d != y.as.d) return True;
            return x.as.d == y.as.d;
        case _HASHMAP_LONGDOUBLE:
            if (x.as.ld != x.as.ld && y.as.ld != y.as.ld) return True;
            return x.as.ld == y.as.ld;
        default:
            return False;
    }
}


void __hashmap_print__(HashMapVariant x) {
    switch (x.type) {
        case _HASHMAP_STRING: {
            if (x.as.str) printf("\x1b[1;32m\"%s\"\x1b[0m", x.as.str);
            else printf("\x1b[1;31m%p\x1b[0m", NULL);
            break;
        }
        case _HASHMAP_POINTER: {
            if (x.as.ptr) printf("\x1b[1;33m%p\x1b[0m", x.as.ptr);
            else printf("\x1b[1;31m%p\x1b[0m", NULL);
            break;
        }
        case _HASHMAP_INT32:
            printf("\x1b[1;35m%d\x1b[0m", x.as.i);
            break;
        case _HASHMAP_INT64:
            printf("\x1b[1;35m%lld\x1b[0m", x.as.l);
            break;
        case _HASHMAP_USIZE:
            printf("\x1b[1;35m%zu\x1b[0m", x.as.u);
            break;
        case _HASHMAP_FLOAT:
            printf("\x1b[1;35m%f\x1b[0m", x.as.f);
            break;
        case _HASHMAP_DOUBLE:
            printf("\x1b[1;35m%lf\x1b[0m", x.as.d);
            break;
        case _HASHMAP_LONGDOUBLE:
            printf("\x1b[1;35m%Lf\x1b[0m", x.as.ld);
            break;
        default:
            printf("\x1b[1;31mnull\x1b[0m");
    }
}


void __hashmap_variant_cleanup__(HashMapVariant x) {
    if (x.type == _HASHMAP_STRING && x.as.str) free(x.as.str);
    // The ownership of memory is user's.
    // >>> if (x.type == _HASHMAP_POINTER && x.as.ptr) free(x.as.ptr);
}


void __hashmap_resize__(HashMap *dict, uint32 n_buckets) {
    n_buckets = __next_pow2__(n_buckets);
    if (n_buckets < HASHMAP_INIT_BUCKETS) n_buckets = HASHMAP_INIT_BUCKETS;
    if (n_buckets == dict->buckets) return;

    _HashMapNode **table = calloc(n_buckets, sizeof(_HashMapNode *));
    if (!table) return;

    for (uint32 i = 0; i < dict->buckets; i++) {
        _HashMapNode *node = dict->table[i];
        while (node) {
            _HashMapNode *next = node->next;
            uint32 index = node->hash & (n_buckets - 1);
            node->next = table[index];
            table[index] = node;
            node = next;
        }
    }

    free(dict->table);
    dict->table = table;
    dict->buckets = n_buckets;
}


usize __hashmap_sizeof__(HashMap *dict) {
    if (!dict) return 0;

    usize total = sizeof(HashMap);
    total = total + (usize)dict->buckets * sizeof(_HashMapNode *);

    for (uint32 i = 0; i < dict->buckets; i++) {
        for (_HashMapNode *node = dict->table[i]; node; node = node->next) {
            total = total + sizeof(_HashMapNode);
            if (node->key.type == _HASHMAP_STRING && node->key.as.str) total = total + strlen(node->key.as.str) + 1;
            if (node->value.type == _HASHMAP_STRING && node->value.as.str) total = total + strlen(node->value.as.str) + 1;
        }
    }

    return total;
}
