#include "enemy.h"
#include <stdlib.h>

Vector2 random_Enemy_direction(Enemy * enemy) {
    // To decide wich direction the enemy will follow
    // we generate a random number within. If we get 0 or 1
    // it turns counterclockwise or clockwise, respectivly. Otherwise
    // the enemy doenst change its moving direction.

    // This gives us a 2/40 chance for change direction.
    int random_decision = rand() % 40;
    if (random_decision == 0) { return Vector2Rotate(enemy->direction, -(PI / 4.0f)); }
    else if (random_decision == 1) { return Vector2Rotate(enemy->direction, (PI / 4.0f)); }
    return enemy->direction;
}

