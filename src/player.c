#include "player.h"
#include "window.h"

const float PLAYER_VELOCITY = 100.0;

Player default_Player() {
    Player player = {
        .position = (Vector2) {
            .x = WINDOW_WIDTH / 2.0f,
            .y = WINDOW_HEIGHT / 2.0f,
        },
        .rect = (Rectangle) {
            .x = WINDOW_WIDTH / 2.0f,
            .y = WINDOW_HEIGHT / 2.0f,
            .width = 25.0,
            .height = 25.0,
        },
        .life = 5,
    };
    return player;
}
