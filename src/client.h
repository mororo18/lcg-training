#ifndef __CLIENT_H_
#define __CLIENT_H_

#include "player.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>

typedef
enum RequestType {
    REQUEST_PLAYER_INFO_UPDATE,
    REQUEST_EMPTY,
} RequestType;

typedef
struct RequestPlayerInfoUpdate {
    PlayerInfo info;
} RequestPlayerInfoUpdate;

typedef
union RequestData {
    RequestPlayerInfoUpdate player_info_update;
} RequestData;

typedef
struct Request {
    RequestType type;
    RequestData data;
} Request;

inline static _Bool is_ready(int fd, short event) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = event;

    return poll(&pfd, 1, 0) > 0;
}

inline static _Bool is_ready_for_writing(int fd) {
    return is_ready(fd, POLLOUT);
}

inline static _Bool is_ready_for_reading(int fd) {
    return is_ready(fd, POLLIN);
}

inline
void send_request(Request request, int fd) {
    ssize_t ret = write(fd, &request, sizeof(request));
    assert(ret != -1);
    assert(ret == sizeof(request));
}

inline
Request recieve_request(int fd) {
    Request request = {};
    ssize_t ret = read(fd, &request, sizeof(request));

    printf("recieved %ld bytes\n", ret);

    if (ret < (ssize_t) sizeof(request)) {
        request.type = REQUEST_EMPTY;
    }

    return request;
}

typedef
enum ClientMode {
    CM_SINGLEPAYER,
    CM_MULTIPLAYER,
} ClientMode;

typedef
struct Client {
    ClientMode mode;
    int fd;
    Player my_player;
} Client;

extern Client g_client;

void init_Client();

#endif // __CLIENT_H_
