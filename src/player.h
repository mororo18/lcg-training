#ifndef _PLAYER_H
#define _PLAYER_H

#include <raylib.h>
#include <raymath.h>

typedef
struct Player {
    Rectangle rect;
    Vector2 position;
    float velocity;
    bool is_alive;
} Player;

Player default_Player();

#endif // _PLAYER_H
