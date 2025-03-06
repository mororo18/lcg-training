#include "bullet.h"
#include <string.h>

const float BULLET_VELOCITY = 200.0;
const size_t INITIAL_ARRAY_CAPACITY = 10;

BulletArray new_BulletArray() {
    BulletArray array = {
        .ptr = calloc(INITIAL_ARRAY_CAPACITY, sizeof(Bullet)),
        .capacity = INITIAL_ARRAY_CAPACITY,
        .lenght = 0,
    };

    if (array.ptr == NULL) {
        err(EXIT_FAILURE, "calloc");
    }

    return array;
}

void increase_capacity_of_BulletArray(BulletArray * array, size_t capacity_increase) {

    if (capacity_increase == 0) {
        return;
    }

    Bullet * new_ptr = calloc(
        array->capacity + capacity_increase,
        sizeof(Bullet)
    );

    memcpy(new_ptr, array->ptr, array->lenght * sizeof(Bullet));
    free(array->ptr);
    array->ptr = new_ptr;
    array->capacity += capacity_increase;
}

void push_to_BulletArray(BulletArray * array, Bullet bullet) {
    if (array->lenght == array->capacity) {
        // Doubles the array capacity
        increase_capacity_of_BulletArray(array, array->capacity);
    }

    array->ptr[array->lenght] = bullet;
    array->lenght++;
}

void remove_from_BulletArray(BulletArray * array, size_t index) {
    if (index >= array->lenght) {
        err(EXIT_FAILURE, "index out of bounds");
    }

    array->ptr[index] = array->ptr[array->lenght-1];
    array->lenght--;
}
