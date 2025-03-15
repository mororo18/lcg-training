#define _XOPEN_SOURCE 700
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include <assert.h>
#include <err.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <raylib.h>
#include <raymath.h>

#include "client.h"
#include "server.h"
#include "window.h"
#include "bullet.h"
#include "player.h"
#include "enemy.h"
#include "bullet_array.h"
#include "enemy_array.h"

#define PORT 1234

Client g_client = {
    .mode = CM_MULTIPLAYER,
    .info = {
        .fd = -1,
    },
};

static void signal_handler(int sig){
    if (sig == SIGINT) {
        printf("Shutting down...\n");
        close(g_client.info.fd);
        exit(0);
    }
}

static struct sigaction sigact = {};

void init_signal_handler(void){
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction*) NULL);
}

void setup_multiplayer_client() {
    struct sockaddr_in servaddr = {};
  
    init_signal_handler();

    // socket create and verification 
    g_client.info.fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (g_client.info.fd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } else {
        printf("Socket successfully created..\n"); 
    }

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    servaddr.sin_port = htons(PORT);

     // connect the client socket to server socket
    if (connect(g_client.info.fd, (struct sockaddr*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    } else {
        printf("connected to the server..\n");
    }
}

uint32_t recieve_player_id_from_server() {
    uint32_t player_id;

    for (;;) {
        if (is_ready_for_reading(g_client.info.fd)) {
            Event event = recieve_event(g_client.info.fd);

            assert(event.type == EVENT_PLAYER_ACCEPTED);
            player_id = (uint32_t) event.data.player_accepted.id;
            printf("Player accepted (client id = %u)\n", player_id);
            break;
        }
    }

    return player_id;
}

/*
int main () {


    for (;;) {
        char buff[128] = {};
        if (is_ready_for_reading(g_client.fd)) {
            ssize_t bytes_read = read(g_client.fd, buff, sizeof(buff));
            if (bytes_read > 0) {
                buff[bytes_read] = '\0';
                printf("read = %s\n", buff);
            }
        }
    }

    return 0;
}
*/

typedef
struct GameState {
    int my_player_id;
    PlayerArray players;
    BulletArray bullets;
    double last_bullet_timestamp;
    EnemyArray enemies;
    bool game_over;
    float dt;
    Music bg_music;
    Sound shot_sound;
    Sound enemy_elimination_sound;
    Sound player_damage_sound;
    Texture2D hear_icon;
} GameState;

static GameState default_GameState() {
    GameState state = {0};
    //state.players = default_Player();
    state.my_player_id = -1;
    //state.bullets = new_BulletArray();
    //state.enemies = new_EnemyArray();
    state.bg_music = LoadMusicStream("assets/music/red-doors-2.mp3");
    state.shot_sound = LoadSound("assets/sound-fx/launches/flaunch.wav");
    state.enemy_elimination_sound = LoadSound("assets/sound-fx/damage/Hurting_The_Robot.wav");
    state.player_damage_sound = LoadSound("assets/sound-fx/damage/damage_grunt_male.wav");

    return state;
}

static void update_client_state(GameState * state) {

    const Vector2 up    = { .x =  0.0, .y = -1.0 };
    const Vector2 down  = { .x =  0.0, .y =  1.0 };
    const Vector2 right = { .x =  1.0, .y =  0.0 };
    const Vector2 left  = { .x = -1.0, .y =  0.0 };

    if (g_client.info.player_info.life <= 0) {
        state->game_over = true;
        return;
    }

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

    Vector2 player_new_position = Vector2Add(
        g_client.info.player_info.position,
        player_movement_direction
    );
    
    if (CheckCollisionPointRec(player_new_position, window_rect)) {
        g_client.info.player_info.position = Vector2Add(
            g_client.info.player_info.position,
            player_movement_direction
        );
    }
    
    /*
    if (state->player.boost_enabled) {
        state->player.remaining_boost_time -= state->dt;

        if (state->player.remaining_boost_time <= 0.0) {
            state->player.remaining_boost_time = 0.0;
            state->player.boost_req_defeats = 0;
            state->player.boost_enabled = false;
        }
    }
    */

    // Update the positions of the Player's bullets and check for
    // collisions with borders
    for (size_t i = 0; i < g_client.my_bullets.lenght; i++) {
        Bullet * bullet = BulletArray_at(&g_client.my_bullets, i);

        // Check collision with borders
        if (CheckCollisionPointRec(bullet->info.position, window_rect)) {
            bullet->info.position = Vector2Add(
                bullet->info.position,
                Vector2Scale(bullet->direction, BULLET_VELOCITY * state->dt)
            );
        } else {
            remove_from_BulletArray(&g_client.my_bullets, (size_t) i);
            i--;
        }
    }
    
    // Fires a new bullet
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Fires a new bullet
        Vector2 aiming_direction = Vector2Subtract(
            GetMousePosition(),
            g_client.info.player_info.position
        );

        Bullet new_bullet = {
            .direction = Vector2Normalize(aiming_direction),
            .info = {
                .id = g_client.bullet_counter,
                .player_id = g_client.info.player_id,
                .position = g_client.info.player_info.position,
            }
        };

        g_client.bullet_counter++;
        push_to_BulletArray(&g_client.my_bullets, new_bullet);

        state->last_bullet_timestamp = GetTime();
        g_client.was_bullet_fired = true;

        // play the sound effect
        PlaySound(state->shot_sound);
    }


    /*
    // Fires a new bullet (boost)
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) 
        && state->player.boost_enabled)
    {
        if (GetTime() - state->last_bullet_timestamp >= 0.4) {
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

    } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Fires a new bullet
        Vector2 aiming_direction = Vector2Subtract(
            GetMousePosition(),
            state->player.position
        );

        Bullet new_bullet = {
            .position = state->player.position,
            .direction = Vector2Normalize(aiming_direction)
        };

        push_to_BulletArray(&state->bullets, new_bullet);

        state->last_bullet_timestamp = GetTime();

        // play the sound effect
        PlaySound(state->shot_sound);
    }



    // Update position of the enemies and check collision
    for (int i = 0; i < (int) state->enemies.lenght; i++) {

        float player_distance = Vector2Distance(
            state->player.position,
            state->enemies.data[i].position
        );

        Vector2 new_direction = {};

        // Enemy will chase the player
        if (player_distance < ENEMY_AWARENESS_RADIUS) {
            new_direction = Vector2Subtract(
                state->player.position,
                state->enemies.data[i].position
            );
        } else {
            new_direction = random_Enemy_direction(state->enemies.data + i);
        }

        state->enemies.data[i].direction = Vector2Normalize(new_direction);

        Vector2 new_position = Vector2Add(
            state->enemies.data[i].position,
            Vector2Scale(state->enemies.data[i].direction, ENEMY_VELOCITY * state->dt)
        );

        // Check collision with borders
        if (CheckCollisionPointRec(new_position, window_rect)) {
            state->enemies.data[i].position = new_position;
        } else {
            state->enemies.data[i].direction =
                Vector2Scale(state->enemies.data[i].direction, -1.0);
        }
        
        // Check collision with Player 
        if (
            CheckCollisionCircleRec(
                state->enemies.data[i].position,
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

    // Check if player died
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
    */

    /*
    printf("Bullet array lenght %lu\n", state->bullets.lenght);
    printf("Bullet array capacity %d\n", BULLET_ARRAY_CAPACITY);
    printf("Enemy array lenght %lu\n",   state->enemies.lenght);
    printf("Enemy array capacity %d\n", ENEMY_ARRAY_CAPACITY);
    */
}

void draw_frame(GameState * state) {
    BeginDrawing();
    {
        ClearBackground(RAYWHITE);

        // Draw my bullets
        for (size_t i = 0; i < g_client.my_bullets.lenght; i++) {
            DrawRectangleV(
                g_client.my_bullets.data[i].info.position,
                (Vector2) { .x=5.0f, .y=5.0f },
                RED
            );
        }

        // Draw bullets of other players
        for (size_t i = 0; i < state->bullets.lenght; i++) {
            DrawRectangleV(
                state->bullets.data[i].info.position,
                (Vector2) { .x=5.0f, .y=5.0f },
                RED
            );
        }
        
        // Draw players 
        for (size_t i = 0; i < state->players.lenght; i++) {
            Player * player = state->players.data + i;
            PlayerInfo player_info = state->players.data[i].info;
            
            // Draw total enemies defeated
            const char * score_fmt = "Player (id=%d) score: %u";
            char str_buff[64] = {};
            sprintf(str_buff, score_fmt,
                    player->id,
                    player_info.enemies_defeated);
            DrawText(str_buff, 15, 60 + (int) i * 30, 20, GRAY);

            Color player_color = RED;
            if (state->players.data[i].id == g_client.info.player_id) {
                player_info = g_client.info.player_info;
                player_color = GREEN;
            }

            Vector2 player_rect_pos = {
                .x = player_info.position.x - PLAYER_RECT.width / 2,
                .y = player_info.position.y - PLAYER_RECT.height / 2,
            };

            DrawRectangle(
                (int) player_rect_pos.x,
                (int) player_rect_pos.y,
                (int) PLAYER_RECT.width,
                (int) PLAYER_RECT.height,
                player_color
            );
            
        }

        // Draw enemies
        for (size_t i = 0; i < state->enemies.lenght; i++) {
            DrawCircleV(
                EnemyArray_get(&state->enemies, i).position,
                ENEMY_RADIUS,
                BLUE
            );
        }

        // Draw remainig life
        for (int i = 0; i < g_client.info.player_info.life; i++) {
            DrawTexture(state->hear_icon, i * state->hear_icon.width, 0, WHITE);
        }



        /*
        // Draw remaining boost time
        if (state->player.boost_enabled) {
            const char * boost_time_fmt = "Remaining boost time: %.2fs";
            sprintf(str_buff, boost_time_fmt, state->player.remaining_boost_time);
            DrawText(str_buff, 15, WINDOW_HEIGHT - 60, 20, GRAY);
        }
        */

        if (state->game_over) {
            DrawText("Congrats! You died!", WINDOW_WIDTH / 3, WINDOW_HEIGHT / 2, 30, GRAY);
        }
    }
    EndDrawing();
}

void send_player_position_to_server(PlayerInfo * player_info) {
    RequestPlayerInfoUpdate pos_update = {
        .info = *player_info,
    };

    Request request = {
        .type = REQUEST_PLAYER_INFO_UPDATE,
        .data = { .player_info_update = pos_update },
    };

    send_request(request, g_client.info.fd);
}

void send_bullets_update_to_server(BulletArray * bullet_array) {
    RequestBulletsInfoUpdate bullets_update = {};
    bullets_update.info.lenght = bullet_array->lenght;

    for (size_t i = 0; i < bullet_array->lenght; i++) {
        BulletInfo * req_bullet = BulletInfoArray_at(&bullets_update.info, i);
        BulletInfo * my_bullet = &BulletArray_at(bullet_array, i)->info;

        *req_bullet = *my_bullet;
    }

    Request request = {
        .type = REQUEST_BULLET_INFO_UPDATE,
        .data = { 
            .bullets_info_update = bullets_update,
        },
    };

    send_request(request, g_client.info.fd);
}

void init_Client() {
    srand((unsigned int) clock());

    InitAudioDevice();
    GameState state = default_GameState();

    if (g_client.mode == CM_MULTIPLAYER) {
        setup_multiplayer_client(); 
        uint32_t player_id = recieve_player_id_from_server();
        state.my_player_id = (int) player_id;

        Player my_player = default_Player();
        my_player.id = (int) player_id;

        g_client.info.player_info = my_player.info;
        g_client.info.player_id = (int) player_id;

        push_to_PlayerArray(&state.players, my_player);
    } else {
        printf("only multiplayer for now.");
        exit(0);
    }

    InitWindow(
        (int) window_rect.width,
        (int) window_rect.height,
        "little candle games - training"
    );
    SetTargetFPS(60);

    state.hear_icon = LoadTexture("assets/heart.png");
    //PlayMusicStream(state.bg_music);

    int64_t start = millis();

    while (!WindowShouldClose()) {
        UpdateMusicStream(state.bg_music);

        state.dt = GetFrameTime();

        if (!state.game_over) {
            update_client_state(&state);
        } else {
            PauseMusicStream(state.bg_music);
        }

        draw_frame(&state);

        int64_t elapsed = millis() - start;
        double elapsed_sec = (double) elapsed / 1000.0;
        // Send the state of the client
        if (elapsed_sec > CLIENT_UPDATE_INTERVAL && is_ready_for_writing(g_client.info.fd)) {
            start = millis();

            send_player_position_to_server(&g_client.info.player_info);
            send_bullets_update_to_server(&g_client.my_bullets);

            if (g_client.was_bullet_fired) {
                g_client.was_bullet_fired = false;

                Request req = {
                    .type = REQUEST_BULLET_SHOT,
                };
                send_request(req, g_client.info.fd);
            }
        }

        // Update other Players positions
        if (is_ready_for_reading(g_client.info.fd)) {
            //printf("recebendo..\t");
            Event event = recieve_event(g_client.info.fd);

            switch (event.type) {
                case EVENT_PLAYER_INFO_UPDATE:
                    {
                        //puts("player info update..");
                        int player_id = event.data.player_info_update.id;
                        PlayerInfo player_new_info = event.data.player_info_update.info;
                        bool player_found = false;


                        for (size_t i = 0; i < state.players.lenght; i++) {
                            if (player_id == state.players.data[i].id) {

                                if (player_new_info.life < state.players.data[i].info.life) {
                                    PlaySound(state.player_damage_sound);
                                }

                                state.players.data[i].info = player_new_info;
                                player_found = true;

                                break;
                            }
                        }

                        if (!player_found) {
                            Player new_player = default_Player();
                            new_player.id = player_id;
                            new_player.info = player_new_info;
                            push_to_PlayerArray(&state.players, new_player);
                        }

                        // Update my player info
                        if (player_id == g_client.info.player_id) {
                            //printf("player score = %d\n", player_new_info.enemies_defeated);
                            g_client.info.player_info.life = player_new_info.life;
                        }

                    } break;
                case EVENT_ENEMIES_POSITION_UPDATE:
                    {
                        //puts("enemies info update..");
                        state.enemies.lenght = 0;

                        for (size_t i = 0; i < MAX_ENEMIES; i++) {
                            Vector2 position = event.data.enemies_position_update.position[i];
                            Enemy enemy = {
                                .position = position,
                            };
                            push_to_EnemyArray(&state.enemies, enemy);
                        }
                    }
                    break;
                case EVENT_BULLETS_INFO_UPDATE:
                    {
                        //puts("bullets info update..");
                        state.bullets.lenght = 0;
                        const size_t info_array_leght = event.data.bullets_info_update.info.lenght;
                        for (size_t i = 0; i < info_array_leght; i++) {
                            BulletInfo info = BulletInfoArray_get(&event.data.bullets_info_update.info, i);

                            if (info.player_id != g_client.info.player_id) {
                                Bullet bullet = {
                                    .info = info,
                                };
                                push_to_BulletArray(&state.bullets, bullet);
                            }
                        }
                    }
                    break;
                case EVENT_ENEMY_KILLED:
                    {
                        //puts("enemy killed..");
                        PlaySound(state.enemy_elimination_sound);
                    } break;
                case EVENT_PLAYERS_WHO_SHOT:
                    {
                        //puts("shots fired..");
                        PlayerIdArray * who_shot = &event.data.players_who_shot.who_shot;
                        for (size_t i = 0; i < who_shot->lenght; i++) {
                            if (PlayerIdArray_get(who_shot, i) != g_client.info.player_id) {
                                PlaySound(state.shot_sound);
                                break;
                            }
                        }
                    } break;
                case EVENT_EMPTY:
                    //puts("empty..");
                    break;
                case EVENT_PLAYER_ACCEPTED:
                    //puts("accepted..");
                    break;
            };

        }

    }

    CloseWindow();

    if (g_client.mode == CM_MULTIPLAYER) {
        close(g_client.info.fd);
    }
}
