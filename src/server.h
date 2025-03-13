#ifndef __SERVER_H_
#define __SERVER_H_

#include "player.h"
#include <raymath.h>
#include <poll.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define MAX_ENEMIES 3

typedef
enum EventType {
    PLAYER_ACCEPTED_EVENT,
    PLAYER_INFO_UPDATE_EVENT,
    ENEMIES_POSITION_UPDATE_EVENT,
    EMPTY_EVENT,
} EventType;

typedef 
struct EnemiesPositionUpdateEvent {
    Vector2 position[MAX_ENEMIES];
} EnemiesPositionUpdateEvent;

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
    PlayerInfoUpdateEvent player_info_update;
    PlayerAcceptedEvent player_accepted;
} EventData;

typedef 
struct Event {
    EventType type;
    EventData data;
} Event;

inline
void send_event(Event event, int fd) {
    ssize_t ret = write(fd, &event, sizeof(event));
    assert(ret != -1);
    assert(ret == sizeof(event));
}

inline
Event recieve_event(int fd) {
    Event event = {};
    ssize_t ret = read(fd, &event, sizeof(event));

    if (ret < (ssize_t) sizeof(event)) {
        event.type = EMPTY_EVENT;
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
