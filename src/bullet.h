#ifndef _BULLET_H
#define _BULLET_H

#include <raymath.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>

typedef
struct Bullet {
    Vector2 position;
    Vector2 direction;
} Bullet;

typedef
struct BulletArray {
    Bullet * ptr;
    size_t capacity;
    size_t lenght;
} BulletArray;

extern const float BULLET_VELOCITY;

BulletArray new_BulletArray();
void increase_capacity_of_BulletArray(BulletArray * array, size_t capacity_increase);
void push_to_BulletArray(BulletArray * array, Bullet bullet);
void remove_from_BulletArray(BulletArray * array, size_t index);

#endif // _BULLET_H
