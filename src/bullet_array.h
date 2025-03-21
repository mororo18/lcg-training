#ifndef __BULLET_ARRAY_H_
#define __BULLET_ARRAY_H_

#include "bullet.h"

#define GENERIC_ARRAY_CAPACITY 512
#define GENERIC_TYPES \
    GTYPE(Bullet)

#include "generic_array.h"
#undef GENERIC_TYPES

#define GENERIC_ARRAY_CAPACITY 512
#define GENERIC_TYPES \
    GTYPE(BulletInfo)

#include "generic_array.h"
#undef GENERIC_TYPES

#endif // __BULLET_ARRAY_H_
