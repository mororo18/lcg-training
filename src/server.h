#ifndef __SERVER_H_
#define __SERVER_H_

#include <time.h>
#include <stdint.h>


inline
static int64_t millis()
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return ((int64_t) now.tv_sec) * 1000 + ((int64_t) now.tv_nsec) / 1000000;
}


#endif // __SERVER_H_
