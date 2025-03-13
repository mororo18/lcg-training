#ifndef _PLAYER_H
#define _PLAYER_H

#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

typedef
struct PlayerInfo {
    Vector2 position;
} PlayerInfo;

typedef
struct Player {
    PlayerInfo info;
    int life;
    int enemies_defeated;
    int boost_req_defeats;
    float remaining_boost_time;
    bool boost_enabled;
    int id;
} Player;

Player default_Player();

extern const float PLAYER_VELOCITY;
extern const float PLAYER_BOOST_TIME;
extern const Rectangle player_rect;


#define PLAYER_ARRAY_CAPACITY 16

typedef
struct PlayerArray {
    Player data[PLAYER_ARRAY_CAPACITY];
    size_t lenght;
} PlayerArray;

void push_to_PlayerArray(PlayerArray * array, Player bullet);
void remove_from_PlayerArray(PlayerArray * array, size_t index);

#endif // _PLAYER_H
