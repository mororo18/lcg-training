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
    int boost_req_defeats;
    float remaining_boost_time;
    bool boost_enabled;
} Player;

Player default_Player();

extern const float PLAYER_VELOCITY;
extern const float PLAYER_BOOST_TIME;

#endif // _PLAYER_H
