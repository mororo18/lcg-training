#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <float.h>
#include <sys/select.h>

#include "utils.h"
#include "window.h"
#include "player.h"
#include "bullet.h"
#include "enemy.h"
#include "packet.h"
#include "server.h"
#include "client.h"
#include "config.h"
#include "player_array.h"
#include "enemy_array.h"
#include "client_array.h"


static void server_signal_handler(int sig){
    if (sig == SIGINT && ((Server*)aux_data)->sockfd != -1) {
        printf("Shutting down...\n");
        close(((Server*)aux_data)->sockfd);

        for (size_t i = 0; i < ((Server*)aux_data)->client_info_array.lenght; i++) {
            ClientInfo * client = ClientInfoArray_at(&((Server*)aux_data)->client_info_array, i);
            close(client->fd);
        }
        exit(0);
    }
}

static void refuse_connection(ClientInfo * new_client) {
    printf("Refusing connection...\n");
    send_packet(&(Packet) {.type=EVENT_PLAYER_REFUSED}, new_client->fd);
    close(new_client->fd);
}

void init_server_socket(Server * server) {
    init_signal_handler(server_signal_handler, server);
    // socket create and verification 
    server->sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (server->sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } else printf("Socket (fd=%d) successfully created..\n", server->sockfd); 

    // assign IP, PORT 
    struct sockaddr_in servaddr = { .sin_family = AF_INET, .sin_addr.s_addr = inet_addr(SERVER_IP), .sin_port = htons(SERVER_PORT), };

    // Binding newly created socket to given IP and verification 
    if ( bind( server->sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } else printf("Socket successfully binded..\n");

    if (listen(server->sockfd, 10)) {
        printf("Listen failed...\n"); 
        exit(0);
    } else printf("Server listening..\n");

    init_packet_queues();

}

void init_server(Server * server) {
    *server = (Server) { .sockfd = -1 };
    init_server_socket(server);
}

static void handle_invalid_client(ClientInfo * client) {
    assert(!is_fd_valid(client->fd));
    printf("Client (fd=%d) removed!\n", client->fd);
    reset_packet_queue(client->fd);
}

static void send_enemies_position_to_client(EnemyArray * enemy_array, ClientInfo * dest) {
    EnemiesPositionUpdateEvent enemies_position_packet = {};

    assert(sizeof(enemies_position_packet.position) / sizeof(enemies_position_packet.position[0]) == enemy_array->lenght);
    for (size_t i = 0; i < enemy_array->lenght; i++) enemies_position_packet.position[i] = EnemyArray_get(enemy_array, i).position;

    send_packet(&(Packet){ .type = EVENT_ENEMIES_POSITION_UPDATE, .data = { .enemies_position_update = enemies_position_packet }, }, dest->fd);
}

static void send_bullets_position_to_client(BulletInfoArray * bullets_info, ClientInfo * dest) {
    BulletsInfoUpdateEvent bullets_info_packet = { .info = { .lenght = bullets_info->lenght} };

    for (size_t i = 0; i < bullets_info->lenght; i++) *BulletInfoArray_at(&bullets_info_packet.info, i) = *BulletInfoArray_at(bullets_info, i);

    send_packet(&(Packet){ .type = EVENT_BULLETS_INFO_UPDATE, .data = { .bullets_info_update_event = bullets_info_packet }, }, dest->fd);
}

static void send_bullets_destroyed_last_to_client(BulletInfoArray * bullets_last_destroyed, ClientInfo * dest) {
    DestroyedBulletsEvent destroyed_bullets = { .bullets = { .lenght = bullets_last_destroyed->lenght } };

    for (size_t i = 0; i < bullets_last_destroyed->lenght; i++) *BulletInfoArray_at(&destroyed_bullets.bullets, i) = *BulletInfoArray_at(bullets_last_destroyed, i);

    send_packet(&(Packet){ .type = EVENT_DESTROYED_BULLETS, .data = { .destroyed_bullets = destroyed_bullets }, }, dest->fd);
}

static void update_server_game_state(Server * server, float elapsed_secs) {
    // Update score of the player
    for (size_t client_i = 0; client_i < server->client_info_array.lenght; client_i++) {
        ClientInfo * client = ClientInfoArray_at(&server->client_info_array, client_i);

        client->player_info.remaining_boost_time = ( client->player_info.boost_enabled ? client->player_info.remaining_boost_time - elapsed_secs :  client->player_info.remaining_boost_time);
        if (client->player_info.boost_enabled && client->player_info.remaining_boost_time <= 0.0) {
            client->player_info.remaining_boost_time = 0.0;
            client->player_info.boost_req_defeats = 0;
            client->player_info.boost_enabled = false;
        }
    }

    server->was_some_enemy_killed = false;
    
    // Update position of the enemies and check collision
    for (size_t i = 0; i <  server->enemies.lenght; i++) {
        Enemy * enemy_i = EnemyArray_at(&server->enemies, i);

        // Check colission with bullets
        bool enemy_killed = false;
        for (size_t bullet_i = 0; bullet_i < server->bullets_info.lenght; bullet_i++) {
            BulletInfo bullet = BulletInfoArray_get(&server->bullets_info, bullet_i);
            if (CheckCollisionPointCircle(bullet.position, enemy_i->position, ENEMY_RADIUS + 5)) {
                enemy_killed = true;
                push_to_BulletInfoArray(&server->bullets_destroyed_last, bullet);
                // Remove bullet
                remove_from_BulletInfoArray(&server->bullets_info, bullet_i--);

                // Update score of the player
                for (size_t client_i = 0; client_i < server->client_info_array.lenght; client_i++) {
                    ClientInfo * client = ClientInfoArray_at(&server->client_info_array, client_i);
                    if (client->fd == bullet.player_id) {
                        client->player_info.enemies_defeated++;
                        
                        // Update total enemies defeated only in normal mode
                        if (!client->player_info.boost_enabled) client->player_info.boost_req_defeats++;

                        // Check for player boost condition
                        if (client->player_info.boost_req_defeats >= PLAYER_BOOST_REQUIRED_DEFEATS && !client->player_info.boost_enabled) {
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
        // TODO: this still can be minimized
        for (size_t client_i = 0; client_i < server->client_info_array.lenght; client_i++) {
            ClientInfo * client = ClientInfoArray_at(&server->client_info_array, client_i);
            if ( CheckCollisionCircleRec( enemy_i->position, ENEMY_RADIUS, (Rectangle){ .x = client->player_info.position.x, .y = client->player_info.position.y, .width = PLAYER_RECT.width, .height = PLAYER_RECT.height}) && client->player_info.life > 0) {
                enemy_killed = true;
                client->player_info.life--;
                break;
            }
        }

        if (enemy_killed) {
            server->was_some_enemy_killed = true;
            // Remove enemy
            remove_from_EnemyArray(&server->enemies, i--);
            continue;
        }

        struct {
            int id;
            float distance;
            Vector2 position;
        } nearest_player = { .id = -1, .distance = FLT_MAX, };

        for (size_t client_j = 0; client_j < server->client_info_array.lenght; client_j++) if (Vector2Distance( ClientInfoArray_at(&server->client_info_array, client_j)->player_info.position, enemy_i->position) < nearest_player.distance && ClientInfoArray_at(&server->client_info_array, client_j)->player_info.life > 0) {
                nearest_player.distance = Vector2Distance( ClientInfoArray_at(&server->client_info_array, client_j)->player_info.position, enemy_i->position);
                nearest_player.id = ClientInfoArray_at(&server->client_info_array, client_j)->fd;
                nearest_player.position = ClientInfoArray_at(&server->client_info_array, client_j)->player_info.position;
            }

        // Enemy will chase the player
        EnemyArray_at(&server->enemies, (size_t) i)->direction = Vector2Normalize(( nearest_player.distance < ENEMY_AWARENESS_RADIUS ? Vector2Subtract( nearest_player.position, enemy_i->position) : random_Enemy_direction(enemy_i)));

        // Check collision with borders
        if (CheckCollisionPointRec(Vector2Add( enemy_i->position, Vector2Scale(enemy_i->direction, ENEMY_VELOCITY * elapsed_secs)), window_rect)) enemy_i->position = Vector2Add( enemy_i->position, Vector2Scale(enemy_i->direction, ENEMY_VELOCITY * elapsed_secs));
        else enemy_i->direction = Vector2Scale(enemy_i->direction, -1.0);
        
    }

    // Spawn enemies
    if (server->enemies.lenght < MAX_ENEMIES) {
        const Vector2 possible_directions[] = { WIN_UP, Vector2Add(WIN_UP, WIN_RIGHT), WIN_RIGHT, Vector2Add(WIN_RIGHT, WIN_DOWN), WIN_DOWN, Vector2Add(WIN_DOWN, WIN_LEFT), WIN_LEFT, Vector2Add(WIN_LEFT, WIN_UP), };
        const size_t new_enemies = MAX_ENEMIES - server->enemies.lenght;
        for (int i = 0; i < (int) new_enemies; i++) push_to_EnemyArray(&server->enemies, (Enemy){ .position = (Vector2) { .x = (float) (rand() % WINDOW_WIDTH), .y = (float) (rand() % WINDOW_HEIGHT), }, .direction = Vector2Normalize(possible_directions[rand() % (int)(sizeof(possible_directions) / sizeof(Vector2))]), });
    }
}

void update_bullets_from_player(BulletInfoArray * updated_bullets, int player_id, Server * server) {

    // Remove all old bullets
    for (size_t i = 0; i < server->bullets_info.lenght; i++) if (BulletInfoArray_get(&server->bullets_info, i).player_id == player_id) remove_from_BulletInfoArray(&server->bullets_info, i--);

    // Add the new ones
    for (size_t i = 0; i < updated_bullets->lenght; i++) push_to_BulletInfoArray(&server->bullets_info, BulletInfoArray_get(updated_bullets, i));
}

void run_server(Server * server) {

    if (server->sockfd == -1) return;

    int64_t start = millis();

    while (true) {
        // Set the fd set to zero
        FD_ZERO(&server->readfds);
        // Add sockfd to the set
        FD_SET(server->sockfd, &server->readfds);

        // Add client sockets to set
        int max_fd = server->sockfd;
        for (size_t i = 0; i < server->client_info_array.lenght; i++) {
            ClientInfo * client = ClientInfoArray_at(&server->client_info_array, i);

            if (!is_fd_valid(client->fd)) {
                handle_invalid_client(client);
                remove_from_ClientInfoArray(&server->client_info_array, i--);
                continue;
            }

            if (client->fd > 0) FD_SET(client->fd, &server->readfds);

            if (client->fd > max_fd) max_fd = client->fd;
        }

        // Wait for an activity
        int activity = select(max_fd + 1, &server->readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }

        // New connection
        if (FD_ISSET(server->sockfd, &server->readfds)) {

            struct sockaddr_in  address = {}; 
            socklen_t addrlen = sizeof(address);

            int new_socket = accept(server->sockfd, (struct sockaddr*)&address, &addrlen);

            if (new_socket < 0) {
                perror("Accept error");
                continue;
            }

            printf("New connection: socket %d\n", new_socket);

            ClientInfo new_client = { .fd = new_socket, .player_info = default_PlayerInfo(), };

            if (server->client_info_array.lenght + 1 > MAX_CLIENTS) refuse_connection(&new_client);
            else {
                assign_packet_queue_to_client(new_client.fd);
                push_to_ClientInfoArray(&server->client_info_array, new_client);
                // Sends the new_client.fd to the client.
                // The fd is used as the id of a player.
                send_packet(&(Packet){ .type = EVENT_PLAYER_ACCEPTED, .data =  { .player_accepted = { .id = new_client.fd, .info = new_client.player_info, } }, }, new_client.fd);
            }
        }


        // Handle client packets
        for (ssize_t i = 0; i < (ssize_t) server->client_info_array.lenght; i++) {
            ClientInfo * client = ClientInfoArray_at(&server->client_info_array, (size_t) i);

            if (!is_fd_valid(client->fd)) {
                handle_invalid_client(client);
                remove_from_ClientInfoArray(&server->client_info_array, (size_t) i--);
                continue;
            }

            if (pending_packets(client->fd, get_packet_queue(client->fd), FD_ISSET(client->fd, &server->readfds))) {
                Packet * client_packet = recieve_packet(get_packet_queue(client->fd));

                //log_packet_type("Server recieved: ", client_packet->type);
                assert(is_request(client_packet->type) || client_packet->type == PACKET_INVALID);
                switch (client_packet->type) {
                    case REQUEST_PLAYER_INFO_UPDATE: client->player_info.position = client_packet->data.req_player_info_update.info.position;
                        break;
                    case REQUEST_BULLET_INFO_UPDATE: update_bullets_from_player(&client_packet->data.req_bullets_info_update.info, client->fd, server);
                        break;
                    case REQUEST_BULLET_SHOT: push_to_PlayerIdArray(&server->who_last_shot, client->fd);
                        break;
                    case REQUEST_EMPTY: {
                            printf("Client disconnected: socket %d\n", client->fd);
                            close(client->fd);
                            reset_packet_queue(client->fd);
                            remove_from_ClientInfoArray(&server->client_info_array, (size_t) i--);
                        } break;
                    case PACKET_INVALID: break;
                    default: assert(false);
                };
            }
        }


        int64_t elapsed = millis() - start;
        if (elapsed > SERVER_BROADCAST_PERIOD_MS) {
            float elapsed_secs = (float) ((double) elapsed / 1000.0);
            // update global game state
            update_server_game_state(server, elapsed_secs);
            start = millis();

            for (ssize_t i = 0; i < (ssize_t) server->client_info_array.lenght; i++) {
                ClientInfo * dest = ClientInfoArray_at(&server->client_info_array, (size_t) i);

                if (!is_ready_for_writing(dest->fd)) continue;

                // Send player's info to client_info_array
                for (ssize_t index = 0; index < (ssize_t) server->client_info_array.lenght; index++) send_packet(&(Packet){ .type = EVENT_PLAYER_INFO_UPDATE, .data = { .player_info_update_event = { .id = ClientInfoArray_at(&server->client_info_array, (size_t) index)->fd, .info = ClientInfoArray_at(&server->client_info_array, (size_t) index)->player_info, } }, }, dest->fd);
                
                // Send enemies' info to clients
                send_enemies_position_to_client(&server->enemies, dest);

                // Send the position of the bullets to clients
                send_bullets_position_to_client(&server->bullets_info, dest);

                // Tell if some enemy was killed
                if (server->was_some_enemy_killed) send_packet(&(Packet){ .type = EVENT_ENEMY_KILLED, }, dest->fd);

                if (server->who_last_shot.lenght > 0) send_packet(&(Packet){ .type = EVENT_PLAYERS_WHO_SHOT, .data = { .players_who_shot = { server->who_last_shot } }, }, dest->fd);

                if (server->bullets_destroyed_last.lenght > 0) send_bullets_destroyed_last_to_client(&server->bullets_destroyed_last, dest);

                send_packet(&(Packet){ .type = dest->player_info.boost_enabled ? EVENT_PLAYER_ENABLE_BOOST : EVENT_PLAYER_DISABLE_BOOST}, dest->fd);
            }

            // Reset after broadcast
            server->who_last_shot.lenght = 0;
            server->bullets_destroyed_last.lenght = 0;
        }

    }
}
