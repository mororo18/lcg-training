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
#include <sys/select.h>

#include "window.h"
#include "player.h"
#include "enemy.h"
#include "server.h"
#include "client.h"

#define GENERIC_TYPES \
    GTYPE(Client)

#include "generic_array.h"

#define PORT 1234
#define MAX_CLIENTS 3

typedef
struct Server {
    int sockfd;
    GenericArray clients;
    EnemyArray enemies;
    fd_set readfds;
} Server;

Server server = {
    .sockfd = -1,
    .clients = {},
    .readfds = {},
};

static void signal_handler(int sig){
    if (sig == SIGINT && server.sockfd != -1) {
        printf("Shutting down...\n");
        close(server.sockfd);

        for (size_t i = 0; i < server.clients.lenght; i++) {
            Client client = ClientArray_get(&server.clients, i);
            close(client.fd);
        }
        exit(0);
    }
}

static struct sigaction sigact = {};

static void init_signal_handler(void){
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *) NULL);
}

static void refuse_connection(Client new_client) {
    printf("Refusing connection...\n");
    const char * refuse_msg = "Connection refused.\n";
    send(new_client.fd, refuse_msg, strlen(refuse_msg), 0);
    close(new_client.fd);
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

}

inline clock_t check_clock_failure(clock_t clk) {
    if (clk == -1) {
        err(EXIT_FAILURE, "clock() failed");
    }

    return clk;
}

void send_player_position_to_client(Player * player, Client * src, Client * dest) {
    PlayerInfoUpdateEvent player_info_update = {
        .id = src->fd,
        .info = player->info,
    };

    Event event = {
        .type = PLAYER_INFO_UPDATE_EVENT,
        .data = {
            .player_info_update = player_info_update
        },
    };

    send_event(event, dest->fd);
}

void send_enemies_position_to_client(EnemyArray * enemy_array, Client * dest) {
    EnemiesPositionUpdateEvent enemies_position_event = {};

    size_t position_array_size =
        sizeof(enemies_position_event.position) / sizeof(enemies_position_event.position[0]);
    assert(position_array_size == enemy_array->lenght);
    for (size_t i = 0; i < enemy_array->lenght; i++) {
        enemies_position_event.position[i] = enemy_array->data[i].position;
    }

    Event event = {
        .type = ENEMIES_POSITION_UPDATE_EVENT,
        .data = {
            .enemies_position_update = enemies_position_event
        },
    };

    send_event(event, dest->fd);
}

void update_game_state() {
    const Vector2 up    = { .x =  0.0, .y = -1.0 };
    const Vector2 down  = { .x =  0.0, .y =  1.0 };
    const Vector2 right = { .x =  1.0, .y =  0.0 };
    const Vector2 left  = { .x = -1.0, .y =  0.0 };

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
        for (size_t i = 0; i < server.clients.lenght; i++) {
            Client client = ClientArray_get(&server.clients, i);

            if (client.fd > 0) {
                FD_SET(client.fd, &server.readfds);
            }

            if (client.fd > max_fd) {
                max_fd = client.fd;
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

            Client new_client = {
                .fd = new_socket,
            };

            if (server.clients.lenght == MAX_CLIENTS) {
                refuse_connection(new_client);
            } else {
                push_to_ClientArray(&server.clients, new_client);
                // Sends the new_client.fd to the client.
                // The fd is used as the id of a player.
                PlayerAcceptedEvent player_accepted = {
                    .id = new_client.fd,
                };

                Event event = {
                    .type = PLAYER_ACCEPTED_EVENT,
                    .data =  {
                        .player_accepted = player_accepted
                    },
                };

                send_event(event, new_client.fd);
            }
        }


        // Handle client requests
        for (ssize_t i = 0; i < (ssize_t) server.clients.lenght; i++) {

            Client client = ClientArray_get(&server.clients, (size_t) i);
            if (FD_ISSET(client.fd, &server.readfds)) {
                Request client_request = recieve_request(client.fd);

                switch (client_request.type) {
                    case REQUEST_PLAYER_INFO_UPDATE:
                        {
                            Vector2 player_new_pos =
                                client_request.data.player_info_update.info.position;
                            server.clients
                                .data[i]
                                .Client_data
                                .my_player.info.position = player_new_pos;
                        } break;
                    case REQUEST_EMPTY:
                        {
                            printf("Client disconnected: socket %d\n", client.fd);
                            close(client.fd);
                            remove_from_ClientArray(&server.clients, (size_t) i);
                            i--;
                        } break;
                };
            }
        }

        // update global game state
        update_game_state();
        
        int64_t elapsed = millis() - start;
        float elapsed_secs = (float) ((double) elapsed / 1000.0);
        //printf("elapsed secs =%.5fs\n", elapsed_secs);
        if (elapsed_secs > broadcast_period) {
            start = millis();

            for (ssize_t i = 0; i < (ssize_t) server.clients.lenght; i++) {
                Client dest = ClientArray_get(&server.clients, (size_t) i);

                if (!is_ready_for_writing(dest.fd)) {
                    continue;
                }

                // Send player's info to clients
                for (ssize_t index = 0; index < (ssize_t) server.clients.lenght; index++) {
                    //printf("(i, j) = (%ld, %ld)\n", i, j);
                    Client src = ClientArray_get(&server.clients, (size_t) index);
                    Player player = src.my_player;
                    send_player_position_to_client(&player, &src, &dest);
                }
                
                // Send enemies' info to clients
                for (ssize_t index = 0; index < (ssize_t) server.enemies.lenght; index++) {
                    send_enemies_position_to_client((EnemyArray *) server.enemies.data, &dest);
                }
            }
        }

    }
}

int main () {
    init_server();
    run_server();
    return 0;
}
