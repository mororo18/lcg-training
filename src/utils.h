#ifndef __UTILS_H_
#define __UTILS_H_

#define _XOPEN_SOURCE 700
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <stdint.h>

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

static inline bool is_fd_valid(int fd) {
    return fcntl(fd, F_GETFL) != -1;
}


inline static int64_t millis() {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return ((int64_t) now.tv_sec) * 1000 + ((int64_t) now.tv_nsec) / 1000000;
}

static struct sigaction sigact = {};
static void * aux_data = NULL;

static void init_signal_handler(void (*handler)(int), void * data){
    signal(SIGPIPE, SIG_IGN);
    aux_data = data;
    sigact.sa_handler = handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *) NULL);
}

#endif // __UTILS_H_
