#ifndef _WINDOW_H
#define _WINDOW_H

#include <raylib.h>
#include <raymath.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 500

static const Rectangle window_rect = {
    .x = 0.,
    .y = 0.,
    .width = WINDOW_WIDTH,
    .height = WINDOW_HEIGHT,
};

static const Vector2 WIN_UP    = { .x =  0.0, .y = -1.0 };
static const Vector2 WIN_DOWN  = { .x =  0.0, .y =  1.0 };
static const Vector2 WIN_RIGHT = { .x =  1.0, .y =  0.0 };
static const Vector2 WIN_LEFT  = { .x = -1.0, .y =  0.0 };



#endif // _WINDOW_H
