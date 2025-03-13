#include "player.h"
#include "window.h"
#include <stdlib.h>
#include <err.h>

const float PLAYER_VELOCITY = 100.0;
const float PLAYER_BOOST_TIME = 5.0; // seconds
const Rectangle player_rect = {
    .x = WINDOW_WIDTH / 2.0f,
    .y = WINDOW_HEIGHT / 2.0f,
    .width = 25.0,
    .height = 25.0,
};

Player default_Player() {
    Player player = {
        .info = (PlayerInfo) {
            .position = (Vector2) {
                .x = WINDOW_WIDTH / 2.0f,
                .y = WINDOW_HEIGHT / 2.0f,
            },
        },
        .life = 5,
        .remaining_boost_time = 1.0,
        .boost_enabled = false,
        .id = -1,
    };
    return player;
}

void push_to_PlayerArray(PlayerArray * array, Player bullet) {
    if (array->lenght == PLAYER_ARRAY_CAPACITY) {
        err(EXIT_FAILURE, "BulletArray is full");
    }

    array->data[array->lenght] = bullet;
    array->lenght++;
}

void remove_from_PlayerArray(PlayerArray * array, size_t index) {
    if (index >= array->lenght) {
        err(EXIT_FAILURE, "index out of bounds");
    }

    array->data[index] = array->data[array->lenght-1];
    array->lenght--;
}
