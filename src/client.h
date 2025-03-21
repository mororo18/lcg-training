#ifndef __CLIENT_H_
#define __CLIENT_H_

#include "player.h"
#include "bullet_array.h"
#include <assert.h>
#include <unistd.h>
#include <poll.h>

typedef
enum ClientMode {
    CM_SINGLEPAYER,
    CM_MULTIPLAYER,
} ClientMode;

typedef
struct ClientInfo {
    int fd;
    int player_id;
    PlayerInfo player_info;
} ClientInfo;

typedef int ClientId;

typedef
struct Client {
    ClientMode mode;
    ClientInfo info;
    BulletArray my_bullets;
    bool was_bullet_fired;
    uint32_t bullet_counter;
} Client;


static const uint32_t CLIENT_UPDATE_RATE = 30;
static const float CLIENT_UPDATE_INTERVAL = 1.0f / (float) CLIENT_UPDATE_RATE;
extern Client g_client;

void init_Client();

#endif // __CLIENT_H_
