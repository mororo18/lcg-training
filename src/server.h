#ifndef __SERVER_H_
#define __SERVER_H_

#include <stdint.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <float.h>
#include <sys/select.h>

#include "player_array.h"
#include "enemy_array.h"
#include "client_array.h"

static const uint32_t SERVER_PORT = 1234;
static const char * SERVER_IP = "0.0.0.0";

static const uint32_t SERVER_BROADCAST_FREQ = 10;
static const uint32_t SERVER_BROADCAST_PERIOD_MS = 1000 / SERVER_BROADCAST_FREQ;

typedef struct Server {
    int sockfd;
    fd_set readfds;
    ClientInfoArray client_info_array;
    EnemyArray enemies;
    BulletInfoArray bullets_info;
    bool was_some_enemy_killed;
    PlayerIdArray who_last_shot;
    BulletInfoArray bullets_destroyed_last;
} Server;

void init_server(Server * server);
void run_server(Server * server);

#endif // __SERVER_H_
