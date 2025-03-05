#include <assert.h>
#include <err.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>

#include "window.h"
#include "bullet.h"
#include "player.h"
#include "enemy.h"

const Rectangle window_rect = {
    .x = 0.,
    .y = 0.,
    .width = WINDOW_WIDTH,
    .height = WINDOW_HEIGHT,
};

typedef
struct GameState {
    Player player;
    BulletArray bullets;
    EnemyArray enemies;
    bool game_over;
} GameState;

GameState default_GameState() {
    GameState state = {0};
    state.player = default_Player();
    state.bullets = new_BulletArray();
    state.enemies = new_EnemyArray();
    return state;
}

void update_GameState(GameState * state) {

    const Vector2 up = { .x = 0.0, .y = -1.0 };
    const Vector2 down = { .x = 0.0, .y = 1.0 };
    const Vector2 right = { .x = 1.0, .y = 0.0 };
    const Vector2 left = { .x = -1.0, .y = 0.0 };

    Vector2 player_movement_direction = Vector2Zero();

    if (IsKeyDown(KEY_W)) {
        player_movement_direction = Vector2Add(
            player_movement_direction,
            up
        );
    }

    if (IsKeyDown(KEY_D)) {
        player_movement_direction = Vector2Add(
            player_movement_direction,
            right
        );
    }

    if (IsKeyDown(KEY_A)) {
        player_movement_direction = Vector2Add(
            player_movement_direction,
            left
        );
    }

    if (IsKeyDown(KEY_S)) {
        player_movement_direction = Vector2Add(
            player_movement_direction,
            down
        );
    }

    player_movement_direction = Vector2Scale(
        Vector2Normalize(player_movement_direction),
        state->player.velocity
    );

    state->player.position = Vector2Add(
        state->player.position,
        player_movement_direction
    );

    state->player.rect.x = state->player.position.x;
    state->player.rect.y = state->player.position.y;

    // Update the positions of the bullets and check for collisions
    for (int i = 0; i < (int) state->bullets.lenght; i++) {
        Vector2 * bullet_pos = &state->bullets.ptr[i].position;
        Vector2 * bullet_direction = &state->bullets.ptr[i].direction;

        // Check elimination enemies
        for (int enemy_index = 0; enemy_index < (int) state->enemies.lenght; enemy_index++) {
            Vector2 enemy_pos = state->enemies.ptr[enemy_index].position;
            if (CheckCollisionPointCircle(*bullet_pos, enemy_pos, ENEMY_RADIUS)) {
                remove_from_EnemyArray(&state->enemies, (size_t) enemy_index);
                enemy_index--;
                remove_from_BulletArray(&state->bullets, (size_t) i);
                i--;
                break;
            }
        }

        if (CheckCollisionPointRec(*bullet_pos, window_rect)) {
            *bullet_pos = Vector2Add(
                *bullet_pos,
                Vector2Scale(*bullet_direction, BULLET_VELOCITY)
            );
        } else {
            remove_from_BulletArray(&state->bullets, (size_t) i);
            i--;
        }
    }

    // Fire a new bullet
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 aiming_direction = Vector2Subtract(
            GetMousePosition(),
            state->player.position
        );

        Bullet new_bullet = {
            .position = state->player.position,
            .direction = Vector2Normalize(aiming_direction)
        };

        push_to_BulletArray(&state->bullets, new_bullet);
    }


    // Update position of the enemies
    for (int i = 0; i < (int) state->enemies.lenght; i++) {
        // To decide wich direction the enemy will follow
        // we generate a random number within. If we get 0 or 1 it turns counterclockwise or clockwise, respectivly. Otherwise
        // the enemy doenst change its moving direction.

        // This gives us a 2/40 chance for change direction.
        float angle = PI / 4.0f;
        const int turn_clockwise = 0;
        const int turn_counterclockwise = 1;
        const int range = 40;

        int random_decision = rand() % range;

        if (random_decision == turn_clockwise) {
            state->enemies.ptr[i].direction =
                Vector2Rotate(state->enemies.ptr[i].direction, -angle);
        } else if (random_decision == turn_counterclockwise) {
            state->enemies.ptr[i].direction =
                Vector2Rotate(state->enemies.ptr[i].direction, angle);
        }


        Vector2 new_position = Vector2Add(
            state->enemies.ptr[i].position,
            Vector2Scale(state->enemies.ptr[i].direction, ENEMY_VELOCITY)
        );

        // Check collision with borders
        if (CheckCollisionPointRec(new_position, window_rect)) {
            state->enemies.ptr[i].position = new_position;
        } else {
            state->enemies.ptr[i].direction =
                Vector2Scale(state->enemies.ptr[i].direction, -1.0);
        }
        
        // Check collision with Player 
        if (CheckCollisionCircleRec(state->enemies.ptr[i].position, ENEMY_RADIUS, state->player.rect)) {
            state->player.is_alive = false;
        }
    }

    if (!state->player.is_alive) {
        state->game_over = true;
        return;
    }

    // Spawn enemies
    if (state->enemies.lenght < MAX_ENEMIES) {
        const Vector2 possible_directions[] = {
            up,
            Vector2Add(up, right),
            right,
            Vector2Add(right, down),
            down,
            Vector2Add(down, left),
            left,
            Vector2Add(left, up),
        };
        const int n_directions = sizeof(possible_directions) / sizeof(Vector2);
        const size_t new_enemies = MAX_ENEMIES - state->enemies.lenght;
        for (int i = 0; i < (int) new_enemies; i++) {

            assert(n_directions == 8);

            Vector2 rand_direction = Vector2Normalize(possible_directions[rand() % n_directions]);

            Enemy new_enemy = {
                .position = (Vector2) {
                    .x = rand() % WINDOW_WIDTH,
                    .y = rand() % WINDOW_HEIGHT,
                },
                .direction = rand_direction,
            };

            push_to_EnemyArray(&state->enemies, new_enemy);
        }

    }

    //printf("Bullet array lenght %lu\n", state->bullets.lenght);
    //printf("Bullet array capacity %lu\n", state->bullets.capacity);
}

int main(void) {

    srand((unsigned int) clock());

    GameState state = default_GameState();
    InitWindow((int) window_rect.width, (int) window_rect.height, "little candle games - trainning");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        if (!state.game_over) {
            update_GameState(&state);
        }

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            // Draw the player
            DrawRectangle(
                (int) state.player.position.x,
                (int) state.player.position.y,
                (int) state.player.rect.width,
                (int) state.player.rect.height,
                RED
            );

            // Draw the bullets
            for (size_t i = 0; i < state.bullets.lenght; i++) {
                DrawRectangleV(
                    state.bullets.ptr[i].position,
                    (Vector2) { .x=5.0f, .y=5.0f },
                    RED
                );
            }

            // Draw the enemies
            for (size_t i = 0; i < state.enemies.lenght; i++) {
                DrawCircleV(
                    state.enemies.ptr[i].position,
                    ENEMY_RADIUS,
                    BLUE
                );
            }

            DrawFPS(10, 10);

            if (state.game_over) {
                DrawText("Congrats! You died!", WINDOW_WIDTH / 3, WINDOW_HEIGHT / 2, 30, GRAY);
            }
        }
        EndDrawing();
    }

    CloseWindow();

    free(state.bullets.ptr);

    return 0;
}
