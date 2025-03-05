#include <err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>

const Rectangle window_rect = {
    .x = 0.,
    .y = 0.,
    .width = 800.,
    .height = 500.,
};

typedef
struct Player {
    Vector2 position;
    float velocity;
} Player;

Player default_Player() {
    Player player = {
        .position = (Vector2) {
            .x = window_rect.width / 2.0f,
            .y = window_rect.height / 2.0f,
        },
        .velocity = 2.0,
    };
    return player;
}

const float BULLET_VELOCITY = 5.0;
typedef
struct Bullet {
    Vector2 position;
    Vector2 direction;
} Bullet;

const size_t INITIAL_ARRAY_CAPACITY = 1;
typedef
struct BulletArray {
    Bullet * ptr;
    size_t capacity;
    size_t lenght;
} BulletArray;

BulletArray new_BulletArray() {
    BulletArray array = {
        .ptr = calloc(INITIAL_ARRAY_CAPACITY, sizeof(Bullet)),
        .capacity = INITIAL_ARRAY_CAPACITY,
        .lenght = 0,
    };

    if (array.ptr == NULL) {
        err(EXIT_FAILURE, "calloc");
    }

    return array;
}

void increase_capacity_of_BulletArray(BulletArray * array, size_t capacity_increase) {

    if (capacity_increase == 0) {
        return;
    }

    Bullet * new_ptr = calloc(
        array->capacity + capacity_increase,
        sizeof(Bullet)
    );

    memcpy(new_ptr, array->ptr, array->lenght * sizeof(Bullet));
    free(array->ptr);
    array->ptr = new_ptr;
    array->capacity += capacity_increase;
}

void push_to_BulletArray(BulletArray * array, Bullet bullet) {
    if (array->lenght == array->capacity) {
        // TODO: Create strategy to increase the capacity of the array
        // more eficiently (less increases over time).
        increase_capacity_of_BulletArray(array, 1);
    }

    array->ptr[array->lenght] = bullet;
    array->lenght++;
}

void remove_from_BulletArray(BulletArray * array, size_t index) {
    if (index >= array->lenght) {
        err(EXIT_FAILURE, "index out of bounds");
    }

    array->ptr[index] = array->ptr[array->lenght-1];
    array->lenght--;
}

typedef
struct GameState {
    Player player;
    BulletArray bullet_array;
} GameState;

GameState default_GameState() {
    GameState state = {0};
    state.player = default_Player();
    state.bullet_array = new_BulletArray();
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

    // Update the positions of the bullets and check for collisions
    for (size_t i = 0; i < state->bullet_array.lenght; i++) {
        Vector2 * bullet_pos = &state->bullet_array.ptr[i].position;
        Vector2 * bullet_direction = &state->bullet_array.ptr[i].direction;

        if (CheckCollisionPointRec(*bullet_pos, window_rect)) {
            *bullet_pos = Vector2Add(
                *bullet_pos,
                Vector2Scale(*bullet_direction, BULLET_VELOCITY)
            );
        } else {
            remove_from_BulletArray(&state->bullet_array, i);
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

        push_to_BulletArray(&state->bullet_array, new_bullet);
    }

    printf("Bullet array lenght %lu\n", state->bullet_array.lenght);
    printf("Bullet array capacity %lu\n", state->bullet_array.capacity);
}

int main(void) {

    GameState state = default_GameState();
    InitWindow((int) window_rect.width, (int) window_rect.height, "little candle games - trainning");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        update_GameState(&state);
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            // Draw the player
            DrawRectangle(
                (int) state.player.position.x,
                (int) state.player.position.y,
                25,
                25,
                RED
            );

            // Draw the bullets
            for (size_t i = 0; i < state.bullet_array.lenght; i++) {
                DrawRectangleV(
                    state.bullet_array.ptr[i].position,
                    (Vector2) { .x=5.0f, .y=5.0f },
                    RED
                );
            }


            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    CloseWindow();

    free(state.bullet_array.ptr);

    return 0;
}
