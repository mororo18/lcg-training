#include "player.h"
#include "window.h"
#include <err.h>


PlayerInfo default_PlayerInfo() {
    return (PlayerInfo) { .life = PLAYER_LIFE, .position = (Vector2) { .x = WINDOW_WIDTH / 2.0f, .y = WINDOW_HEIGHT / 2.0f, }, };
}
Player default_Player() {
    return (Player){ .info = default_PlayerInfo(), .id = -1, };
}
