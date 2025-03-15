#ifdef GENERIC_TYPES

#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include "stacktrace.h"


#define GENERIC_ARRAY_CAPACITY 32

#define GTYPE(type) \
typedef \
struct { \
    type data[GENERIC_ARRAY_CAPACITY]; \
    size_t lenght; \
} type##Array;
GENERIC_TYPES
#undef GTYPE

#define GTYPE(type) \
    static inline \
    type type##Array_get(type##Array * array, size_t index) { \
        if (index >= array->lenght) { \
            printf("out fo boudss\n"); \
            print_stacktrace(); \
            exit(1); \
        } \
        return array->data[index]; \
    } \
\
    static inline \
    type * type##Array_at(type##Array * array, size_t index) { \
        if (index >= array->lenght) { \
            printf("out fo boudss\n"); \
            print_stacktrace(); \
            exit(1); \
        } \
        return &array->data[index]; \
    } \
\
static inline \
void push_to_##type##Array(type##Array * array, type item) { \
    if (array->lenght == GENERIC_ARRAY_CAPACITY) { \
        print_stacktrace(); \
        err(EXIT_FAILURE, "BulletArray is full"); \
    } \
    array->data[array->lenght] = item; \
    array->lenght++; \
} \
\
static inline \
void remove_from_##type##Array(type##Array * array, size_t index) { \
    if (index >= array->lenght) { \
        print_stacktrace(); \
        err(EXIT_FAILURE, "index out of bounds"); \
    } \
    array->data[index] = array->data[array->lenght-1]; \
    array->lenght--; \
}
GENERIC_TYPES
#undef GTYPE

#endif // GENERIC_TYPES
