#include "player.h"
#include "window.h"

const float PLAYER_VELOCITY = 100.0;
const float PLAYER_BOOST_TIME = 5.0; // seconds

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
        .remaining_boost_time = 1.0,
        .boost_enabled = false,
    };
    return player;
}
