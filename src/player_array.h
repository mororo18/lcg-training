#ifndef __PLAYER_ARRAY_H_
#define __PLAYER_ARRAY_H_

#include "player.h"

#define GENERIC_TYPES \
    GTYPE(PlayerId)

#include "generic_array.h"
#undef GENERIC_TYPES

#define GENERIC_ARRAY_CAPACITY 16
#define GENERIC_TYPES \
    GTYPE(Player)

#include "generic_array.h"
#undef GENERIC_TYPES

#endif // __PLAYER_ARRAY_H_
