#ifndef _PLAYER_H
#define _PLAYER_H

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

typedef
struct PlayerInfo {
    Vector2 position;
    int enemies_defeated;
    int boost_req_defeats;
    float remaining_boost_time;
    bool boost_enabled;
    int life;
} PlayerInfo;

inline static
void log_player_info(const PlayerInfo *info) {
    printf("[PLAYER INFO]\n");
    printf("    Position: (%.2f, %.2f)\n", info->position.x, info->position.y);
    printf("    Enemies Defeated: %d\n", info->enemies_defeated);
    printf("    Life: %d\n", info->life);
}

typedef int PlayerId;

typedef
struct Player {
    PlayerInfo info;
    PlayerId id;
} Player;

Player default_Player();
PlayerInfo default_PlayerInfo();

static const int PLAYER_LIFE = 7;
static const float PLAYER_VELOCITY = 100.0;
static const float PLAYER_BOOST_TIME = 5.0; // seconds
static const int PLAYER_BOOST_REQUIRED_DEFEATS = 7;

static const Rectangle PLAYER_RECT = {
    .width = 25.0,
    .height = 25.0,
};


#endif // _PLAYER_H
