#ifndef _ENEMY_H
#define _ENEMY_H

#include <raymath.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>

typedef
struct Enemy {
    Vector2 position;
    Vector2 direction;
} Enemy;

typedef
struct EnemyArray {
    Enemy * ptr;
    size_t capacity;
    size_t lenght;
} EnemyArray;

extern const float ENEMY_VELOCITY;
extern const float ENEMY_RADIUS;
extern const size_t MAX_ENEMIES;

EnemyArray new_EnemyArray();
void increase_capacity_of_EnemyArray(EnemyArray * array, size_t capacity_increase);
void push_to_EnemyArray(EnemyArray * array, Enemy bullet);
void remove_from_EnemyArray(EnemyArray * array, size_t index);

#endif // _ENEMY_H
