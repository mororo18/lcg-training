#ifndef _BULLET_H
#define _BULLET_H

#include <raymath.h>
#include <stdint.h>
#include <err.h>
#include "player.h"

typedef uint32_t BulletId;

typedef struct BulletInfo {
    PlayerId player_id;
    BulletId id;
    Vector2 position;
} BulletInfo;

typedef struct Bullet {
    Vector2 direction;
    BulletInfo info;
} Bullet;

static const float BULLET_VELOCITY = 200.0;

#endif // _BULLET_H
