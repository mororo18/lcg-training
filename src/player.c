#include "player.h"
#include "window.h"

Player default_Player() {
    Player player = {
        .position = (Vector2) {
            .x = WINDOW_WIDTH / 2.0f,
            .y = WINDOW_HEIGHT / 2.0f,
        },
        .velocity = 2.0,
        .rect = (Rectangle) {
            .x = WINDOW_WIDTH / 2.0f,
            .y = WINDOW_HEIGHT / 2.0f,
            .width = 25.0,
            .height = 25.0,
        },
        .is_alive = true,
    };
    return player;
}
