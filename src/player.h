#ifndef _PLAYER_H
#define _PLAYER_H

#include <raylib.h>
#include <raymath.h>

typedef
struct Player {
    Rectangle rect;
    Vector2 position;
    int life;
    int enemies_defeated;
} Player;

Player default_Player();

extern const float PLAYER_VELOCITY;

#endif // _PLAYER_H
