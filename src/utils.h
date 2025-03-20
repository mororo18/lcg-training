#ifndef __UTILS_H_
#define __UTILS_H_

#include <poll.h>

inline static _Bool is_ready(int fd, short event) {
    struct pollfd pfd = {};
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

#endif // __UTILS_H_
