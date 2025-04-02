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

#include "packet.h"
#include "client.h"
#include "server.h"
#include "window.h"
#include "bullet.h"
#include "player.h"
#include "enemy.h"
#include "utils.h"
#include "bullet_array.h"
#include "enemy_array.h"

typedef struct GameState {
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
    Texture2D heart_icon;
} GameState;

static void init_game_state(GameState * state) {
    *state = (GameState) {};
    state->bg_music = LoadMusicStream("assets/music/red-doors-2.mp3");
    state->shot_sound = LoadSound("assets/sound-fx/launches/flaunch.wav");
    state->enemy_elimination_sound = LoadSound("assets/sound-fx/damage/Hurting_The_Robot.wav");
    state->player_damage_sound = LoadSound("assets/sound-fx/damage/damage_grunt_male.wav");
    state->heart_icon = LoadTexture("assets/heart.png");
}

static void client_signal_handler(int sig){
    if (sig == SIGINT) {
        printf("Shutting down...\n");
        close(((Client*)aux_data)->info.fd);
        exit(0);
    }
}

static PlayerAcceptedEvent wait_for_server_approval(Client * client) {
    int64_t start = millis();
    PacketQueue * client_queue = get_packet_queue(client->info.fd);
    while (true) {
        if (pending_packets(client->info.fd, client_queue, is_ready_for_reading(client->info.fd))) {
            Packet * packet = recieve_packet(client_queue);

            if (packet->type == EVENT_PLAYER_ACCEPTED) {
                printf("Player accepted (client id = %u)\n", packet->data.player_accepted.id);
                return packet->data.player_accepted;
            }
        }

        if (millis() - start > CLIENT_APPROVAL_TIMEOUT_MS) {
            printf("Server approval TIMEDOUT(%ldms)\n", CLIENT_APPROVAL_TIMEOUT_MS);
            exit(0);
        }
    }
}

static void connect_client_to_server(Client * client, GameState * state) {
    struct sockaddr_in servaddr = {};
  
    // socket create and verification 
    client->info.fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (client->info.fd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } else {
        printf("Socket successfully created..\n"); 
    }

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(SERVER_PORT);

    if (connect(client->info.fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    } else {
        printf("connected to the server..\n");
    }

    init_packet_queues();
    assign_packet_queue_to_client(client->info.fd);

    PlayerAcceptedEvent player_accepted = wait_for_server_approval(client);

    client->info.player_id = player_accepted.id;
    client->info.player_info = player_accepted.info;

    Player my_player = { .id = player_accepted.id, .info = player_accepted.info, };

    push_to_PlayerArray(&state->players, my_player);
}

static void update_client_game_state(Client * client, GameState * state) {

    // TODO: put this in window.h
    const Vector2 up    = { .x =  0.0, .y = -1.0 };
    const Vector2 down  = { .x =  0.0, .y =  1.0 };
    const Vector2 right = { .x =  1.0, .y =  0.0 };
    const Vector2 left  = { .x = -1.0, .y =  0.0 };

    if (client->info.player_info.life <= 0) {
        state->game_over = true;
        client->my_bullets.lenght = 0;
        return;
    }

    Vector2 player_movement_direction = Vector2Zero();

    if (IsKeyDown(KEY_W)) player_movement_direction = Vector2Add(player_movement_direction, up);

    if (IsKeyDown(KEY_D)) player_movement_direction = Vector2Add(player_movement_direction, right);

    if (IsKeyDown(KEY_A)) player_movement_direction = Vector2Add(player_movement_direction, left);

    if (IsKeyDown(KEY_S)) player_movement_direction = Vector2Add(player_movement_direction, down);

    player_movement_direction = Vector2Scale(Vector2Normalize(player_movement_direction), PLAYER_VELOCITY * state->dt);

    Vector2 player_new_position = Vector2Add(client->info.player_info.position, player_movement_direction);
    
    if (CheckCollisionPointRec(player_new_position, window_rect)) client->info.player_info.position = Vector2Add(client->info.player_info.position, player_movement_direction);
    
    // Update the positions of the Player's bullets and check for
    // collisions with borders
    for (size_t i = 0; i < client->my_bullets.lenght; i++) {
        // Check collision with borders
        if (CheckCollisionPointRec(BulletArray_at(&client->my_bullets, i)->info.position, window_rect)) BulletArray_at(&client->my_bullets, i)->info.position = Vector2Add( BulletArray_at(&client->my_bullets, i)->info.position, Vector2Scale(BulletArray_at(&client->my_bullets, i)->direction, BULLET_VELOCITY * state->dt));
        else remove_from_BulletArray(&client->my_bullets, (size_t) i--);
    }
    
    // Fires a new bullet (boost)
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && client->info.player_info.boost_enabled) {
        if (GetTime() - state->last_bullet_timestamp >= 0.1) {
            Vector2 aiming_direction = Vector2Subtract(GetMousePosition(), client->info.player_info.position);

            Bullet new_bullet = { .info = { .id = client->bullet_counter, .player_id = client->info.player_id, .position = client->info.player_info.position, }, .direction = Vector2Normalize(aiming_direction) };

            state->last_bullet_timestamp = GetTime();
            client->bullet_counter++;
            push_to_BulletArray(&client->my_bullets, new_bullet);
            PlaySound(state->shot_sound);
        }

    // Fires a new bullet
    } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 aiming_direction = Vector2Subtract(GetMousePosition(), client->info.player_info.position);

        Bullet new_bullet = { .direction = Vector2Normalize(aiming_direction), .info = { .id = client->bullet_counter, .player_id = client->info.player_id, .position = client->info.player_info.position, } };

        client->bullet_counter++;
        push_to_BulletArray(&client->my_bullets, new_bullet);

        state->last_bullet_timestamp = GetTime();
        client->was_bullet_fired = true;

        // play the sound effect
        PlaySound(state->shot_sound);
    }
}

static void draw_frame(Client * client, GameState * state) {
    BeginDrawing(); {
        ClearBackground(RAYWHITE);

        // Draw my bullets
        for (size_t i = 0; i < client->my_bullets.lenght; i++) DrawRectangleV(client->my_bullets.data[i].info.position, (Vector2) { .x=5.0f, .y=5.0f }, RED);

        // Draw bullets of other players
        for (size_t i = 0; i < state->bullets.lenght; i++) DrawRectangleV(state->bullets.data[i].info.position, (Vector2) { .x=5.0f, .y=5.0f }, RED);
        
        // Draw players 
        for (size_t i = 0; i < state->players.lenght; i++) {

            DrawText(TextFormat("Player (id=%d) score: %u", state->players.data[i].id, state->players.data[i].info.enemies_defeated), 15, 60 + (int) i * 30, 20, GRAY);

            Vector2 player_rect_pos = { .x = (state->players.data[i].id == client->info.player_id ? client->info.player_info : state->players.data[i].info).position.x - PLAYER_RECT.width / 2, .y = (state->players.data[i].id == client->info.player_id ? client->info.player_info : state->players.data[i].info).position.y - PLAYER_RECT.height / 2, };

            DrawRectangle((int) player_rect_pos.x, (int) player_rect_pos.y, (int) PLAYER_RECT.width, (int) PLAYER_RECT.height, (state->players.data[i].id == client->info.player_id ? GREEN : RED));
            
        }

        // Draw enemies
        for (size_t i = 0; i < state->enemies.lenght; i++) DrawCircleV(EnemyArray_get(&state->enemies, i).position, ENEMY_RADIUS, BLUE);

        // Draw remainig life
        for (int i = 0; i < client->info.player_info.life; i++) DrawTexture(state->heart_icon, i * state->heart_icon.width, 0, WHITE);

        // Draw boost indicator
        if (client->info.player_info.boost_enabled) DrawText("BOOOSSTT!!", 15, WINDOW_HEIGHT - 30, 20, GRAY);

        if (state->game_over) DrawText("Congrats! You died!", WINDOW_WIDTH / 3, WINDOW_HEIGHT / 2, 30, GRAY);
    } EndDrawing();
}

static void send_player_position_to_server(PlayerInfo * player_info, int fd) {
    RequestPlayerInfoUpdate pos_update = { .info = *player_info, };
    Packet packet = { .type = REQUEST_PLAYER_INFO_UPDATE, .data = { .req_player_info_update = pos_update }, };
    send_packet(&packet, fd);
}

static void send_bullets_update_to_server(BulletArray * bullet_array, int fd) {
    RequestBulletsInfoUpdate bullets_update = { .info = { .lenght = bullet_array->lenght } };

    for (size_t i = 0; i < bullet_array->lenght; i++) *(BulletInfoArray_at(&bullets_update.info, i)) = BulletArray_at(bullet_array, i)->info;

    Packet packet = { .type = REQUEST_BULLET_INFO_UPDATE, .data = { .req_bullets_info_update = bullets_update, }, };
    send_packet(&packet, fd);
}

void init_client(Client * client, ClientMode mode) {
    srand((unsigned int) clock());
    init_signal_handler(client_signal_handler, client);
    InitAudioDevice();

    InitWindow( (int) window_rect.width, (int) window_rect.height, "little candle games - training");
    SetTargetFPS(60);

    *client = (Client) { .mode = mode, .info = {.fd = -1} };
}

void run_client(Client * client) {

    GameState state = {};
    init_game_state(&state);

    if (client->mode == CM_MULTIPLAYER) connect_client_to_server(client, &state); 
    else {
        printf("only multiplayer for now.");
        exit(0);
    }

    PlayMusicStream(state.bg_music);

    int64_t start = millis();
    while (!WindowShouldClose()) {
        UpdateMusicStream(state.bg_music);

        state.dt = GetFrameTime();

        if (!state.game_over) update_client_game_state(client, &state);
        else                  PauseMusicStream(state.bg_music);

        draw_frame(client, &state);

        int64_t elapsed = millis() - start;
        // Send the state of the client to the server
        if (elapsed > CLIENT_UPDATE_INTERVAL_MS && is_ready_for_writing(client->info.fd)) {
            start = millis();

            send_player_position_to_server(&client->info.player_info, client->info.fd);
            send_bullets_update_to_server(&client->my_bullets, client->info.fd);

            if (client->was_bullet_fired) {
                client->was_bullet_fired = false;

                Packet req = { .type = REQUEST_BULLET_SHOT, };
                send_packet(&req, client->info.fd);
            }
        }

        // Recieve packets
        bool is_fd_ready = is_ready_for_reading(client->info.fd);
        PacketQueue * client_queue = get_packet_queue(client->info.fd);
        if (pending_packets(client->info.fd, client_queue, is_fd_ready)) {
            Packet * packet = recieve_packet(client_queue);

            assert(is_event(packet->type));
            switch (packet->type) {
                case EVENT_PLAYER_INFO_UPDATE: {
                        //puts("player info update..");
                        int player_id = packet->data.player_info_update_event.id;
                        PlayerInfo player_new_info = packet->data.player_info_update_event.info;
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
                        if (player_id == client->info.player_id) client->info.player_info.life = player_new_info.life;

                    } break;
                case EVENT_ENEMIES_POSITION_UPDATE: {
                        state.enemies.lenght = 0;

                        for (size_t i = 0; i < MAX_ENEMIES; i++) {
                            Vector2 position = packet->data.enemies_position_update.position[i];
                            Enemy enemy = {
                                .position = position,
                            };
                            push_to_EnemyArray(&state.enemies, enemy);
                        }
                    } break;
                case EVENT_BULLETS_INFO_UPDATE: {
                        state.bullets.lenght = 0;
                        const size_t info_array_leght = packet->data.bullets_info_update_event.info.lenght;
                        for (size_t i = 0; i < info_array_leght; i++) {
                            BulletInfo info = BulletInfoArray_get(&packet->data.bullets_info_update_event.info, i);

                            if (info.player_id != client->info.player_id) {
                                Bullet bullet = {
                                    .info = info,
                                };
                                push_to_BulletArray(&state.bullets, bullet);
                            }
                        }
                    } break;
                case EVENT_DESTROYED_BULLETS: {
                        for (size_t i = 0; i < packet->data.destroyed_bullets.bullets.lenght; i++)
                            for (size_t bullet_index = 0; bullet_index < client->my_bullets.lenght; bullet_index++) {
                                if (BulletInfoArray_at(&packet->data.destroyed_bullets.bullets, i)->id == BulletArray_at(&client->my_bullets, bullet_index)->info.id && BulletInfoArray_at(&packet->data.destroyed_bullets.bullets, i)->player_id == BulletArray_at(&client->my_bullets, bullet_index)->info.player_id) {
                                    remove_from_BulletArray(&client->my_bullets, bullet_index);
                                    break;
                                }
                            }

                    } break;
                case EVENT_ENEMY_KILLED: PlaySound(state.enemy_elimination_sound);
                    break;
                case EVENT_PLAYERS_WHO_SHOT: {
                        for (size_t i = 0; i < packet->data.players_who_shot.who_shot.lenght; i++) {
                            if (PlayerIdArray_get(&packet->data.players_who_shot.who_shot, i) != client->info.player_id) PlaySound(state.shot_sound);
                            if (PlayerIdArray_get(&packet->data.players_who_shot.who_shot, i) != client->info.player_id) break;
                        }
                    } break;
                case EVENT_PLAYER_ENABLE_BOOST: client->info.player_info.boost_enabled = true;
                    break;
                case EVENT_PLAYER_DISABLE_BOOST: client->info.player_info.boost_enabled = false;
                    break;
                case EVENT_EMPTY: break;
                case EVENT_PLAYER_ACCEPTED: break;
                default: assert(false);
            };

        }


        if (!is_fd_valid(client->info.fd)) break;
    }

    CloseWindow();

    if (client->mode == CM_MULTIPLAYER) close(client->info.fd);
}
