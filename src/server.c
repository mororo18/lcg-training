#define _XOPEN_SOURCE 700
#include <assert.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <float.h>
#include <sys/select.h>

#include "window.h"
#include "player.h"
#include "bullet.h"
#include "enemy.h"
#include "packet.h"
#include "server.h"
#include "client.h"
#include "player_array.h"
#include "enemy_array.h"
#include "client_array.h"

#define PORT 1234
#define MAX_CLIENTS 5

typedef
struct Server {
    int sockfd;
    ClientInfoArray client_info_array;
    EnemyArray enemies;
    BulletInfoArray bullets_info;
    fd_set readfds;
    bool was_some_enemy_killed;
    PlayerIdArray who_last_shot;
    BulletInfoArray bullets_destroyed_last;
} Server;

Server server = {
    .sockfd = -1,
    .client_info_array = {},
    .readfds = {},
};

static void server_signal_handler(int sig){
    if (sig == SIGINT && server.sockfd != -1) {
        printf("Shutting down...\n");
        close(server.sockfd);

        for (size_t i = 0; i < server.client_info_array.lenght; i++) {
            ClientInfo * client = ClientInfoArray_at(&server.client_info_array, i);
            close(client->fd);
        }
        exit(0);
    }
}

static struct sigaction sigact = {};

static void init_signal_handler(void){
    sigact.sa_handler = server_signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *) NULL);
}

static void refuse_connection(ClientInfo * new_client) {
    printf("Refusing connection...\n");
    send_packet(&(Packet) {.type=EVENT_PLAYER_REFUSED}, new_client->fd);
    close(new_client->fd);
}

void init_server() {
    init_signal_handler();
    // socket create and verification 
    server.sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (server.sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } else {
        printf("Socket (fd=%d) successfully created..\n", server.sockfd); 
    }
    //
    // assign IP, PORT 
    struct sockaddr_in servaddr = {};
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification 
    if (
        bind(
            server.sockfd,
            (struct sockaddr *)&servaddr,
            sizeof(servaddr)
        )
    ) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } else {
        printf("Socket successfully binded..\n");
    }

    if (listen(server.sockfd, 3)) {
        printf("Listen failed...\n"); 
        exit(0);
    } else {
        printf("Server listening..\n");
    }

    init_packet_queues();

}

inline clock_t check_clock_failure(clock_t clk) {
    if (clk == -1) {
        err(EXIT_FAILURE, "clock() failed");
    }

    return clk;
}

void send_player_info_to_client(PlayerInfo * player_info, ClientInfo * src, ClientInfo * dest) {
    PlayerInfoUpdateEvent player_info_update = {
        .id = src->fd,
        .info = *player_info,
    };

    Packet packet = {
        .type = EVENT_PLAYER_INFO_UPDATE,
        .data = {
            .player_info_update_event = player_info_update
        },
    };

    send_packet(&packet, dest->fd);
}

void send_enemies_position_to_client(EnemyArray * enemy_array, ClientInfo * dest) {
    EnemiesPositionUpdateEvent enemies_position_packet = {};

    size_t position_array_size =
        sizeof(enemies_position_packet.position) / sizeof(enemies_position_packet.position[0]);
    assert(position_array_size == enemy_array->lenght);
    for (size_t i = 0; i < enemy_array->lenght; i++) {
        enemies_position_packet.position[i] = EnemyArray_get(enemy_array, i).position;
    }

    Packet packet = {
        .type = EVENT_ENEMIES_POSITION_UPDATE,
        .data = {
            .enemies_position_update = enemies_position_packet
        },
    };

    send_packet(&packet, dest->fd);
}

void send_bullets_position_to_client(BulletInfoArray * bullets_info, ClientInfo * dest) {
    BulletsInfoUpdateEvent bullets_info_packet = {};
    bullets_info_packet.info.lenght = bullets_info->lenght;

    for (size_t i = 0; i < bullets_info->lenght; i++) {
        BulletInfo * server_bullet = BulletInfoArray_at(bullets_info, i);
        BulletInfo * packet_bullet = BulletInfoArray_at(&bullets_info_packet.info, i);
        *packet_bullet = *server_bullet;
    }

    Packet packet = {
        .type = EVENT_BULLETS_INFO_UPDATE,
        .data = {
            .bullets_info_update_event = bullets_info_packet
        },
    };

    send_packet(&packet, dest->fd);
}

void send_last_shots_to_client(PlayerIdArray * who_last_shot, ClientInfo * dest) {
    Packet packet = {
        .type = EVENT_PLAYERS_WHO_SHOT,
        .data = {
            .players_who_shot = { *who_last_shot }
        },
    };

    send_packet(&packet, dest->fd);
}

void send_bullets_destroyed_last_to_client(BulletInfoArray * bullets_last_destroyed, ClientInfo * dest) {
    //printf("sending %lu last destroyed bullets\n", bullets_last_destroyed->lenght);
    DestroyedBulletsEvent destroyed_bullets = {};

    destroyed_bullets.bullets.lenght = bullets_last_destroyed->lenght;
    for (size_t i = 0; i < bullets_last_destroyed->lenght; i++) {
        BulletInfo * server_bullet = BulletInfoArray_at(bullets_last_destroyed, i);
        BulletInfo * packet_bullet = BulletInfoArray_at(&destroyed_bullets.bullets, i);
        *packet_bullet = *server_bullet;
    }

    Packet packet = {
        .type = EVENT_DESTROYED_BULLETS,
        .data = {
            .destroyed_bullets = destroyed_bullets
        },
    };

    send_packet(&packet, dest->fd);
}

void update_game_state(float elapsed_secs) {
    const Vector2 up    = { .x =  0.0, .y = -1.0 };
    const Vector2 down  = { .x =  0.0, .y =  1.0 };
    const Vector2 right = { .x =  1.0, .y =  0.0 };
    const Vector2 left  = { .x = -1.0, .y =  0.0 };

    /*
    // Update the positions of the bullets and check for collisions
    for (int i = 0; i < (int) server.bullets.lenght; i++) {
        Vector2 * bullet_pos = &server.bullets.data[i].info.position;
        Vector2 * bullet_direction = &server.bullets.data[i].info.direction;

        // Check enemy elimination 
        for (int enemy_index = 0; enemy_index < (int) server.enemies.lenght; enemy_index++) {
            Vector2 enemy_pos = EnemyArray_get(&server.enemies, (size_t) enemy_index).position;
            if (CheckCollisionPointCircle(*bullet_pos, enemy_pos, ENEMY_RADIUS)) {
                // Remove enemy
                remove_from_EnemyArray(&server.enemies, (size_t) enemy_index);
                enemy_index--;

                // Remove bullet
                remove_from_BulletArray(&server.bullets, (size_t) i);
                i--;

                // Update total enemies defeated
                state->player.enemies_defeated++;

                // Update total enemies defeated only in normal mode
                if (!state->player.boost_enabled) {
                    state->player.boost_req_defeats++;
                }

                // Check for player boost condition
                if (state->player.boost_req_defeats >= 10
                    && !state->player.boost_enabled) {
                    state->player.boost_enabled = true;
                    state->player.remaining_boost_time = PLAYER_BOOST_TIME;
                }

                // Play sound effects
                PlaySound(state->enemy_elimination_sound);

                break;
            }
        }

        // Check collision with borders
        if (CheckCollisionPointRec(*bullet_pos, window_rect)) {
            *bullet_pos = Vector2Add(
                *bullet_pos,
                Vector2Scale(*bullet_direction, BULLET_VELOCITY * elapsed_secs)
            );
        } else {
            remove_from_BulletArray(&server.bullets, (size_t) i);
            i--;
        }
    }
    */

    // Update score of the player
    for (size_t client_i = 0; client_i < server.client_info_array.lenght; client_i++) {
        ClientInfo * client = ClientInfoArray_at(&server.client_info_array, client_i);

        if (client->player_info.boost_enabled) {
            client->player_info.remaining_boost_time -= elapsed_secs;

            if (client->player_info.remaining_boost_time <= 0.0) {
                client->player_info.remaining_boost_time = 0.0;
                client->player_info.boost_req_defeats = 0;
                client->player_info.boost_enabled = false;
            }
        }
    }

    server.was_some_enemy_killed = false;
    
    // Update position of the enemies and check collision
    for (size_t i = 0; i <  server.enemies.lenght; i++) {
        Enemy * enemy_i = EnemyArray_at(&server.enemies, i);

        // Check colission with bullets
        bool enemy_killed = false;
        for (size_t bullet_i = 0; bullet_i < server.bullets_info.lenght; bullet_i++) {
            BulletInfo bullet = BulletInfoArray_get(&server.bullets_info, bullet_i);
            if (CheckCollisionPointCircle(bullet.position, enemy_i->position, ENEMY_RADIUS + 5)) {
                enemy_killed = true;
                push_to_BulletInfoArray(&server.bullets_destroyed_last, bullet);
                // Remove bullet
                remove_from_BulletInfoArray(&server.bullets_info, bullet_i);

                // Update score of the player
                for (size_t client_i = 0; client_i < server.client_info_array.lenght; client_i++) {
                    ClientInfo * client = ClientInfoArray_at(&server.client_info_array, client_i);
                    if (client->fd == bullet.player_id) {
                        client->player_info.enemies_defeated++;
                        
                        // Update total enemies defeated only in normal mode
                        if (!client->player_info.boost_enabled) {
                            client->player_info.boost_req_defeats++;
                        }

                        // Check for player boost condition
                        if (client->player_info.boost_req_defeats >= PLAYER_BOOST_REQUIRED_DEFEATS
                                && !client->player_info.boost_enabled) {
                            client->player_info.boost_enabled = true;
                            client->player_info.remaining_boost_time = PLAYER_BOOST_TIME;
                        }

                        break;
                    }
                }

                break;
            }
        }

        // Check collision with players
        for (size_t client_i = 0; client_i < server.client_info_array.lenght; client_i++) {
            ClientInfo * client = ClientInfoArray_at(&server.client_info_array, client_i);
            bool player_alive = client->player_info.life > 0;
            Rectangle player_rect = PLAYER_RECT;
            player_rect.x = client->player_info.position.x;
            player_rect.y = client->player_info.position.y;
            if (
                CheckCollisionCircleRec(
                    enemy_i->position,
                    ENEMY_RADIUS,
                    player_rect
                )
                && player_alive
            ) {
                enemy_killed = true;
                client->player_info.life--;
                break;
            }
        }

        if (enemy_killed) {
            server.was_some_enemy_killed = true;
            // Remove enemy
            remove_from_EnemyArray(&server.enemies, i);
            i--;
            continue;
        }

        struct {
            int id;
            float distance;
            Vector2 position;
        } nearest_player = {
            .id = -1,
            .distance = FLT_MAX,
        };

        for (size_t client_j = 0; client_j < server.client_info_array.lenght; client_j++) {
            ClientInfo * client = ClientInfoArray_at(&server.client_info_array, client_j);

            float player_distance = Vector2Distance(
                client->player_info.position,
                enemy_i->position
            );
            bool player_alive = client->player_info.life > 0;
            if (player_distance < nearest_player.distance && player_alive) {
                nearest_player.distance = player_distance;
                nearest_player.id = client->fd;
                nearest_player.position = client->player_info.position;
            }
        }
        //assert(nearest_player.id != -1 || server.client_info_array.lenght == 0);

        Vector2 new_direction = {};

        // Enemy will chase the player
        if (nearest_player.distance < ENEMY_AWARENESS_RADIUS) {
            new_direction = Vector2Subtract(
                nearest_player.position,
                enemy_i->position
            );
        } else {
            new_direction = random_Enemy_direction(enemy_i);
        }

        EnemyArray_at(&server.enemies, (size_t) i)->direction =
            Vector2Normalize(new_direction);

        Vector2 new_position = Vector2Add(
            enemy_i->position,
            Vector2Scale(enemy_i->direction, ENEMY_VELOCITY * elapsed_secs)
        );

        // Check collision with borders
        if (CheckCollisionPointRec(new_position, window_rect)) {
            enemy_i->position = new_position;
        } else {
            enemy_i->direction = Vector2Scale(enemy_i->direction, -1.0);
        }
        
        // Check collision with Player 
        /*
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
        */
    }

    // Spawn enemies
    if (server.enemies.lenght < MAX_ENEMIES) {
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
        const size_t new_enemies = MAX_ENEMIES - server.enemies.lenght;
        for (int i = 0; i < (int) new_enemies; i++) {
            Vector2 rand_direction = Vector2Normalize(possible_directions[rand() % n_directions]);

            Enemy new_enemy = {
                .position = (Vector2) {
                    .x = rand() % WINDOW_WIDTH,
                    .y = rand() % WINDOW_HEIGHT,
                },
                .direction = rand_direction,
            };

            push_to_EnemyArray(&server.enemies, new_enemy);
        }
    }
}

void update_bullets_from_player(BulletInfoArray * updated_bullets, int player_id) {

    // Remove all old bullets
    for (size_t i = 0; i < server.bullets_info.lenght; i++) {
        BulletInfo server_bullet = BulletInfoArray_get(&server.bullets_info, i);

        if (server_bullet.player_id == player_id) {
            remove_from_BulletInfoArray(&server.bullets_info, i);
            i--;
        }

    }

    // Add the new ones
    for (size_t i = 0; i < updated_bullets->lenght; i++) {
        BulletInfo player_bullet = BulletInfoArray_get(updated_bullets, i);
        push_to_BulletInfoArray(&server.bullets_info, player_bullet);
    }
}

void run_server() {

    if (server.sockfd == -1) {
        return;
    }

    int64_t start = millis();
    float broadcast_period = 1.0f / 15.0f;

    for (;;) {
        // Set the fd set to zero
        FD_ZERO(&server.readfds);
        // Add sockfd to the set
        FD_SET(server.sockfd, &server.readfds);

        // Add client sockets to set
        int max_fd = server.sockfd;
        for (size_t i = 0; i < server.client_info_array.lenght; i++) {
            ClientInfo * client = ClientInfoArray_at(&server.client_info_array, i);

            if (client->fd > 0) {
                FD_SET(client->fd, &server.readfds);
            }

            if (client->fd > max_fd) {
                max_fd = client->fd;
            }
        }

        // Wait for an activity
        int activity = select(max_fd + 1, &server.readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }

        // New connection
        if (FD_ISSET(server.sockfd, &server.readfds)) {

            struct sockaddr_in  address = {}; 
            socklen_t addrlen = sizeof(address);

            int new_socket = accept(server.sockfd, (struct sockaddr*)&address, &addrlen);

            if (new_socket < 0) {
                perror("Accept error");
                continue;
            }

            printf("New connection: socket %d\n", new_socket);

            ClientInfo new_client = {
                .fd = new_socket,
                .player_info = default_PlayerInfo(),
            };

            if (server.client_info_array.lenght + 1 > MAX_CLIENTS) {
                refuse_connection(&new_client);
            } else {
                assign_packet_queue_to_client(new_client.fd);
                push_to_ClientInfoArray(&server.client_info_array, new_client);
                // Sends the new_client.fd to the client.
                // The fd is used as the id of a player.
                PlayerAcceptedEvent player_accepted = {
                    .id = new_client.fd,
                    .info = new_client.player_info,
                };

                Packet packet = {
                    .type = EVENT_PLAYER_ACCEPTED,
                    .data =  {
                        .player_accepted = player_accepted
                    },
                };

                send_packet(&packet, new_client.fd);
            }
        }


        // Handle client packets
        //size_t total_recieved_bytes = 0;
        for (ssize_t i = 0; i < (ssize_t) server.client_info_array.lenght; i++) {
            ClientInfo * client = ClientInfoArray_at(&server.client_info_array, (size_t) i);
            PacketQueue * client_queue = get_packet_queue(client->fd);
            bool is_fd_ready = FD_ISSET(client->fd, &server.readfds);
            if (pending_packets(client->fd, client_queue, is_fd_ready)) {
                Packet client_packet = {};
                recieve_packet(&client_packet, client_queue);

                assert(is_request(client_packet.type));
                //log_packet_type("Server recieved: ", client_packet.type);
                switch (client_packet.type) {
                    case REQUEST_PLAYER_INFO_UPDATE:
                        {
                            PlayerInfo player_new_info =
                                client_packet.data.req_player_info_update.info;
                            //int player_current_life = client->player_info.life;
                            //int player_current_score = client->player_info.enemies_defeated;

                            client->player_info.position = player_new_info.position;
                            //client->player_info.life = player_current_life;
                            //client->player_info.enemies_defeated = player_current_score;
                            /*
                            printf("Player pos = (%.2f, %.2f)\n",
                                    player_new_info.position.x,
                                    player_new_info.position.y);
                                    */
                        } break;
                    case REQUEST_BULLET_INFO_UPDATE:
                        {
                            BulletInfoArray * updated_bullets =
                                &client_packet.data.req_bullets_info_update.info;
                            update_bullets_from_player(updated_bullets, client->fd);
                        } break;
                    case REQUEST_BULLET_SHOT:
                        {
                            push_to_PlayerIdArray(&server.who_last_shot, client->fd);
                        } break;
                    case REQUEST_EMPTY:
                        {
                            printf("Client disconnected: socket %d\n", client->fd);
                            close(client->fd);
                            reset_packet_queue(client->fd);
                            remove_from_ClientInfoArray(&server.client_info_array, (size_t) i);
                            i--;
                        } break;
                };
            }
        }


        int64_t elapsed = millis() - start;
        float elapsed_secs = (float) ((double) elapsed / 1000.0);
        //printf("elapsed secs =%.5fs\n", elapsed_secs);
        if (elapsed_secs > broadcast_period) {
            // update global game state
            update_game_state(elapsed_secs);
            start = millis();

            for (ssize_t i = 0; i < (ssize_t) server.client_info_array.lenght; i++) {
                ClientInfo * dest = ClientInfoArray_at(&server.client_info_array, (size_t) i);

                if (!is_ready_for_writing(dest->fd)) {
                    continue;
                }

                // Send player's info to client_info_array
                for (ssize_t index = 0; index < (ssize_t) server.client_info_array.lenght; index++) {
                    //printf("(i, j) = (%ld, %ld)\n", i, j);
                    ClientInfo * src = ClientInfoArray_at(&server.client_info_array, (size_t) index);
                    send_player_info_to_client(&src->player_info, src, dest);
                }
                
                // Send enemies' info to clients
                send_enemies_position_to_client(&server.enemies, dest);

                // Send the position of the bullets to clients
                send_bullets_position_to_client(&server.bullets_info, dest);

                // Tell if some enemy was killed
                if (server.was_some_enemy_killed) {
                    Packet packet = {
                        .type = EVENT_ENEMY_KILLED,
                    };
                    send_packet(&packet, dest->fd);
                }

                if (server.who_last_shot.lenght > 0) {
                    send_last_shots_to_client(&server.who_last_shot, dest);
                }

                if (server.bullets_destroyed_last.lenght > 0) {
                    send_bullets_destroyed_last_to_client(&server.bullets_destroyed_last, dest);
                }

                {
                    Packet packet_boost = {};

                    if (dest->player_info.boost_enabled) {
                        packet_boost.type = EVENT_PLAYER_ENABLE_BOOST;
                    } else {
                        packet_boost.type = EVENT_PLAYER_DISABLE_BOOST;
                    }
                    send_packet(&packet_boost, dest->fd);
                }
            }

            // Reset after broadcast
            server.who_last_shot.lenght = 0;
            server.bullets_destroyed_last.lenght = 0;
        }

    }
}

int main () {
    init_server();
    run_server();
    return 0;
}
