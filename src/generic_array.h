#ifndef __GENERIC_ARRAY_H_
#define __GENERIC_ARRAY_H_

#ifdef GENERIC_TYPES

#include <stdlib.h>
#include <stdio.h>
#include <err.h>

union GenericType {
#define GTYPE(type) type type##_data;
    GENERIC_TYPES
#undef GTYPE
};

#define GENERIC_ARRAY_CAPACITY 512
typedef
struct {
    union GenericType data[128];
    size_t lenght;
} GenericArray;

inline static GenericArray new_GenericArray() {
    GenericArray array = {
        .data = {},
        .lenght = 0,
    };
    return array;
}

#define GTYPE(type) \
    type type##Array_get(GenericArray * array, size_t index) { \
        if (index >= array->lenght) { \
            printf("out fo boudss\n"); \
            exit(1); \
        } \
        return array->data[index].type##_data; \
    } \
\
void push_to_##type##Array(GenericArray * array, type item) { \
    if (array->lenght == GENERIC_ARRAY_CAPACITY) { \
        err(EXIT_FAILURE, "BulletArray is full"); \
    } \
    array->data[array->lenght].type##_data = item; \
    array->lenght++; \
} \
\
void remove_from_##type##Array(GenericArray * array, size_t index) { \
    if (index >= array->lenght) { \
        err(EXIT_FAILURE, "index out of bounds"); \
    } \
    array->data[index] = array->data[array->lenght-1]; \
    array->lenght--; \
}
GENERIC_TYPES
#undef GTYPE

/*
int main () {
    GenericArray foo_array;
    foo_array.data[0].Foo_data = (Foo) { .a = 10.0 };
    foo_array.length = 1;

    printf("foo = %.1f\n", FooArray_get(&foo_array, 0).a);
}
*/

#endif // GENERIC_TYPES
#endif // __GENERIC_ARRAY_H_
