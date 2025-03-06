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
    float dt;
    Music bg_music;
    Sound shot_sound;
    Sound enemy_elimination_sound;
    Sound player_damage_sound;
} GameState;

GameState default_GameState() {
    GameState state = {0};
    state.player = default_Player();
    state.bullets = new_BulletArray();
    state.enemies = new_EnemyArray();
    state.bg_music = LoadMusicStream("assets/music/red-doors-2.mp3");
    state.shot_sound = LoadSound("assets/sound-fx/launches/flaunch.wav");
    state.enemy_elimination_sound = LoadSound("assets/sound-fx/damage/Hurting_The_Robot.wav");
    state.player_damage_sound = LoadSound("assets/sound-fx/damage/damage_grunt_male.wav");

    return state;
}

void update_GameState(GameState * state) {

    const Vector2 up    = { .x =  0.0, .y = -1.0 };
    const Vector2 down  = { .x =  0.0, .y =  1.0 };
    const Vector2 right = { .x =  1.0, .y =  0.0 };
    const Vector2 left  = { .x = -1.0, .y =  0.0 };

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
        PLAYER_VELOCITY * state->dt
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

        // Check enemy elimination 
        for (int enemy_index = 0; enemy_index < (int) state->enemies.lenght; enemy_index++) {
            Vector2 enemy_pos = state->enemies.ptr[enemy_index].position;
            if (CheckCollisionPointCircle(*bullet_pos, enemy_pos, ENEMY_RADIUS)) {
                // Remove enemy
                remove_from_EnemyArray(&state->enemies, (size_t) enemy_index);
                enemy_index--;

                // Remove bullet
                remove_from_BulletArray(&state->bullets, (size_t) i);
                i--;

                // Update total enemies defeated
                state->player.enemies_defeated++;
                
                // Play sound effects
                PlaySound(state->enemy_elimination_sound);

                break;
            }
        }

        // Check collision with borders
        if (CheckCollisionPointRec(*bullet_pos, window_rect)) {
            *bullet_pos = Vector2Add(
                *bullet_pos,
                Vector2Scale(*bullet_direction, BULLET_VELOCITY * state->dt)
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

        // play the sound effect
        PlaySound(state->shot_sound);
    }


    // Update position of the enemies
    for (int i = 0; i < (int) state->enemies.lenght; i++) {

        float player_distance = Vector2Distance(
            state->player.position,
            state->enemies.ptr[i].position
        );

        Vector2 new_direction = {};

        // Enemy will chase the player
        if (player_distance < ENEMY_AWARENESS_RADIUS) {
            new_direction = Vector2Subtract(
                state->player.position,
                state->enemies.ptr[i].position
            );
        } else {
            new_direction = random_Enemy_direction(state->enemies.ptr + i);
        }

        state->enemies.ptr[i].direction = Vector2Normalize(new_direction);

        Vector2 new_position = Vector2Add(
            state->enemies.ptr[i].position,
            Vector2Scale(state->enemies.ptr[i].direction, ENEMY_VELOCITY * state->dt)
        );

        // Check collision with borders
        if (CheckCollisionPointRec(new_position, window_rect)) {
            state->enemies.ptr[i].position = new_position;
        } else {
            state->enemies.ptr[i].direction =
                Vector2Scale(state->enemies.ptr[i].direction, -1.0);
        }
        
        // Check collision with Player 
        if (
            CheckCollisionCircleRec(
                state->enemies.ptr[i].position,
                ENEMY_RADIUS,
                state->player.rect
            )
        ) {
            state->player.life--;

            remove_from_EnemyArray(&state->enemies, (size_t) i);
            i--;
            PlaySound(state->player_damage_sound);
        }
    }

    if (state->player.life <= 0) {
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

    InitWindow((int) window_rect.width, (int) window_rect.height, "little candle games - trainning");
    InitAudioDevice();

    SetTargetFPS(60);

    GameState state = default_GameState();
    const Texture2D hear_icon = LoadTexture("assets/heart.png");
    PlayMusicStream(state.bg_music);

    while (!WindowShouldClose()) {
        UpdateMusicStream(state.bg_music);

        state.dt = GetFrameTime();

        if (!state.game_over) {
            update_GameState(&state);
        } else {
            PauseMusicStream(state.bg_music);
        }


        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            // Draw player
            DrawRectangle(
                (int) state.player.position.x,
                (int) state.player.position.y,
                (int) state.player.rect.width,
                (int) state.player.rect.height,
                RED
            );

            // Draw bullets
            for (size_t i = 0; i < state.bullets.lenght; i++) {
                DrawRectangleV(
                    state.bullets.ptr[i].position,
                    (Vector2) { .x=5.0f, .y=5.0f },
                    RED
                );
            }

            // Draw enemies
            for (size_t i = 0; i < state.enemies.lenght; i++) {
                DrawCircleV(
                    state.enemies.ptr[i].position,
                    ENEMY_RADIUS,
                    BLUE
                );
            }

            // Draw total enemies defeated
            const char * score_fmt = "Enemies defeated: %u";
            char str_buff[64] = {};
            sprintf(str_buff, score_fmt, state.player.enemies_defeated);
            DrawText(str_buff, 15, 60, 20, GRAY);

            // Draw remainig life
            for (int i = 0; i < state.player.life; i++) {
                DrawTexture(hear_icon, i * hear_icon.width, 0, WHITE);
            }

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
