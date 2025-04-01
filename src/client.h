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
static const uint32_t CLIENT_UPDATE_INTERVAL_MS = 1000 / CLIENT_UPDATE_RATE;
static const int64_t CLIENT_APPROVAL_TIMEOUT_MS = 3000;

void init_client(Client * client, ClientMode mode);
void run_client(Client * client);

#endif // __CLIENT_H_
