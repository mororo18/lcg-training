#include "bullet.h"
#include <string.h>

const float BULLET_VELOCITY = 200.0;

BulletArray new_BulletArray() {
    BulletArray array = {
        .data = {},
        .lenght = 0,
    };

    return array;
}

void push_to_BulletArray(BulletArray * array, Bullet bullet) {
    if (array->lenght == BULLET_ARRAY_CAPACITY) {
        err(EXIT_FAILURE, "BulletArray is full");
    }

    array->data[array->lenght] = bullet;
    array->lenght++;
}

void remove_from_BulletArray(BulletArray * array, size_t index) {
    if (index >= array->lenght) {
        err(EXIT_FAILURE, "index out of bounds");
    }

    array->data[index] = array->data[array->lenght-1];
    array->lenght--;
}
