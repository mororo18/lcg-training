#include "enemy.h"
#include <stdlib.h>

const float ENEMY_AWARENESS_RADIUS = 150.0;
const float ENEMY_VELOCITY = 70.0;
const float ENEMY_RADIUS = 10;

Vector2 random_Enemy_direction(Enemy * enemy) {
    // To decide wich direction the enemy will follow
    // we generate a random number within. If we get 0 or 1
    // it turns counterclockwise or clockwise, respectivly. Otherwise
    // the enemy doenst change its moving direction.

    // This gives us a 2/40 chance for change direction.
    float angle = PI / 4.0f;
    const int turn_clockwise = 0;
    const int turn_counterclockwise = 1;
    const int range = 40;

    int random_decision = rand() % range;

    if (random_decision == turn_clockwise) {
        return Vector2Rotate(enemy->direction, -angle);
    } else if (random_decision == turn_counterclockwise) {
        return Vector2Rotate(enemy->direction, angle);
    }

    return enemy->direction;
}

/*
EnemyArray new_EnemyArray() {
    EnemyArray array = {
        .data = {},
        .lenght = 0,
    };

    return array;
}

void push_to_EnemyArray(EnemyArray * array, Enemy bullet) {
    if (array->lenght == ENEMY_ARRAY_CAPACITY) {
        err(EXIT_FAILURE, "EnemyArray is full");
    }

    array->data[array->lenght] = bullet;
    array->lenght++;
}

void remove_from_EnemyArray(EnemyArray * array, size_t index) {
    if (index >= array->lenght) {
        err(EXIT_FAILURE, "index out of bounds");
    }

    array->data[index] = array->data[array->lenght-1];
    array->lenght--;
}
*/
