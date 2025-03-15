#ifndef _BULLET_H
#define _BULLET_H

#include <raymath.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>
#include "player.h"

// #define BULLET_ARRAY_CAPACITY 1024

typedef uint32_t BulletId;

typedef
struct BulletInfo {
    PlayerId player_id;
    BulletId id;
    Vector2 position;
} BulletInfo;

typedef
struct Bullet {
    Vector2 direction;
    BulletInfo info;
} Bullet;

static const float BULLET_VELOCITY = 200.0;
/*
typedef
struct BulletArray {
    Bullet data[BULLET_ARRAY_CAPACITY];
    size_t lenght;
} BulletArray;

extern const float BULLET_VELOCITY;

BulletArray new_BulletArray();
void push_to_BulletArray(BulletArray * array, Bullet bullet);
void remove_from_BulletArray(BulletArray * array, size_t index);
*/

#endif // _BULLET_H
