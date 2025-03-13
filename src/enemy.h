#ifndef _ENEMY_H
#define _ENEMY_H

#include <raymath.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>

#define ENEMY_ARRAY_CAPACITY 128

typedef
struct Enemy {
    Vector2 position;
    Vector2 direction;
} Enemy;

typedef
struct EnemyArray {
    Enemy data[ENEMY_ARRAY_CAPACITY];
    size_t lenght;
} EnemyArray;

extern const float ENEMY_VELOCITY;
extern const float ENEMY_RADIUS;
extern const float ENEMY_AWARENESS_RADIUS;

Vector2 random_Enemy_direction(Enemy * enemy);
EnemyArray new_EnemyArray();
void push_to_EnemyArray(EnemyArray * array, Enemy bullet);
void remove_from_EnemyArray(EnemyArray * array, size_t index);

#endif // _ENEMY_H
