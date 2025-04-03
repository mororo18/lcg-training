#ifdef GENERIC_TYPES

#include <stdlib.h>
#include <stdio.h>
#include <err.h>


#ifndef GENERIC_ARRAY_CAPACITY
#define GENERIC_ARRAY_CAPACITY 32
#endif

#define GTYPE(type) \
typedef struct { \
    type data[GENERIC_ARRAY_CAPACITY]; \
    size_t lenght; \
} type##Array;
GENERIC_TYPES
#undef GTYPE

#define GTYPE(type) \
    static inline type type##Array_get(type##Array * array, size_t index) { \
        if (index >= array->lenght) { err(EXIT_FAILURE, "index out of bounds"); } \
        return array->data[index]; \
    } \
\
    static inline type * type##Array_at(type##Array * array, size_t index) { \
        if (index >= array->lenght) { err(EXIT_FAILURE, "index out of bounds"); } \
        return &array->data[index]; \
    } \
\
static inline void push_to_##type##Array(type##Array * array, type item) { \
    if (array->lenght == GENERIC_ARRAY_CAPACITY) { err(EXIT_FAILURE, "BulletArray is full"); } \
    array->data[array->lenght++] = item; \
} \
\
static inline void remove_from_##type##Array(type##Array * array, size_t index) { \
    if (index >= array->lenght) { err(EXIT_FAILURE, "index out of bounds"); } \
    array->data[index] = array->data[array->lenght-- -1]; \
}
GENERIC_TYPES
#undef GTYPE

#ifdef GENERIC_ARRAY_CAPACITY
#undef GENERIC_ARRAY_CAPACITY
#endif

#endif // GENERIC_TYPES
