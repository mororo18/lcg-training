#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/select.h>
#include "commom_state.h"

typedef
struct Client {
    int fd;
} Client;

#define GENERIC_TYPES \
    GTYPE(Client)

#include "generic_array.h"

#define PORT 1234
#define MAX_CLIENTS 3

typedef
struct Server {
    int sockfd;
    GenericArray clients;
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

struct sigaction sigact = {};

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

void run_server() {

    if (server.sockfd == -1) {
        return;
    }

    for (;;) {
        // Set the fd set to zero
        FD_ZERO(&server.readfds);
        // Add sockfd to the set
        FD_SET(server.sockfd, &server.readfds);

        // Add client sockets to read set
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
                char * welcome = "welcome!!!\n";
                push_to_ClientArray(&server.clients, new_client);
                write(new_client.fd, welcome, strlen(welcome));
            }
        }


        char buff[128] = {};
        // Handle client messages
        for (ssize_t i = 0; i < (ssize_t) server.clients.lenght; i++) {

            Client client = ClientArray_get(&server.clients, (size_t) i);
            if (FD_ISSET(client.fd, &server.readfds)) {
                ssize_t bytes_read = read(client.fd, buff, sizeof(buff));

                printf("bytes read = %lu\n", bytes_read);
                if (bytes_read <= 0) {
                    printf("Client disconnected: socket %d\n", client.fd);
                    close(client.fd);
                    remove_from_ClientArray(&server.clients, (size_t) i);
                    i--;
                } else {
                    buff[bytes_read] = '\0';
                    printf("Received: %s\n", buff);
                    send(client.fd, buff, (size_t) bytes_read, 0);
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
