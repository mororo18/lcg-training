#ifndef __SERVER_H_
#define __SERVER_H_

#include "player.h"
#include "player_array.h"
#include "bullet_array.h"
#include <raymath.h>
#include <poll.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define MAX_ENEMIES 3

typedef
enum EventType {
    EVENT_EMPTY,
    EVENT_PLAYER_ACCEPTED,
    EVENT_PLAYER_REFUSED,
    EVENT_PLAYER_INFO_UPDATE,
    EVENT_PLAYERS_WHO_SHOT,
    EVENT_ENEMIES_POSITION_UPDATE,
    EVENT_ENEMY_KILLED,
    EVENT_BULLETS_INFO_UPDATE,
    EVENT_DESTROYED_BULLETS,
} EventType;

typedef
struct DestroyedBulletsEvent {
    BulletInfoArray bullets;
} DestroyedBulletsEvent;

typedef
struct PlayersWhoShotEvent {
    PlayerIdArray who_shot;
} PlayersWhoShotEvent;

typedef 
struct EnemiesPositionUpdateEvent {
    Vector2 position[MAX_ENEMIES];
} EnemiesPositionUpdateEvent;

typedef 
struct BulletsInfoUpdateEvent {
    BulletInfoArray info;
} BulletsInfoUpdateEvent;

typedef 
struct PlayerInfoUpdateEvent {
    int id;
    PlayerInfo info;
} PlayerInfoUpdateEvent;

typedef 
struct PlayerAcceptedEvent {
    int id;
} PlayerAcceptedEvent;

typedef
union EventData {
    EnemiesPositionUpdateEvent enemies_position_update;
    BulletsInfoUpdateEvent bullets_info_update;
    PlayerInfoUpdateEvent player_info_update;
    PlayerAcceptedEvent player_accepted;
    PlayersWhoShotEvent players_who_shot;
    DestroyedBulletsEvent destroyed_bullets;
} EventData;

typedef 
struct Event {
    EventType type;
    EventData data;
} Event;

inline static
void send_event(Event event, int fd) {
    ssize_t ret = write(fd, &event, sizeof(event));
    assert(ret != -1);
    assert(ret == sizeof(event));
}

inline static
Event recieve_event(int fd) {
    Event event = {};
    ssize_t ret = read(fd, &event, sizeof(event));
    assert(ret != -1);

    if (ret < (ssize_t) sizeof(event)) {
        event.type = EVENT_EMPTY;
    }

    return event;
}

inline
static int64_t millis()
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return ((int64_t) now.tv_sec) * 1000 + ((int64_t) now.tv_nsec) / 1000000;
}


#endif // __SERVER_H_
