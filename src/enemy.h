#ifndef _ENEMY_H
#define _ENEMY_H

#include <raymath.h>
#include <stdint.h>
#include <err.h>

typedef struct Enemy {
    Vector2 position;
    Vector2 direction;
} Enemy;

static const float ENEMY_AWARENESS_RADIUS = 150.0;
static const float ENEMY_VELOCITY = 70.0;
static const float ENEMY_RADIUS = 10;


Vector2 random_Enemy_direction(Enemy * enemy);
#endif // _ENEMY_H
