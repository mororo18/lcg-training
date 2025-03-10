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

#define PORT 1234
#define MAX_CLIENTS 2
struct sigaction sigact = {};
int sockfd = -1;

int clients_fd[MAX_CLIENTS];
int clients_count = 0;

static void signal_handler(int sig){
    if (sig == SIGINT) {
        printf("Shutting down...\n");
        if (sockfd != -1) {
            close(sockfd);
        }
        exit(0);
    }
}

void init_signal_handler(void){
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *)NULL);
}

int main () {

    init_signal_handler();
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } else {
        printf("Socket (fd=%d) successfully created..\n", sockfd); 
    }

    // assign IP, PORT 
    struct sockaddr_in servaddr = {};
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } else {
        printf("Socket successfully binded..\n");
    }

    if (listen(sockfd, 3)) {
        printf("Listen failed...\n"); 
        exit(0);
    } else {
        printf("Server listening..\n");
    }

    // Accept the data packet from client and verification 
    /*
    struct sockaddr_in  cli = {}; 
    socklen_t len = sizeof(cli);
    int connfd = accept(sockfd, (struct sockaddr*)&cli, &len); 
    printf("connfd = %d\n", connfd);
    if (connfd < 0) { 
        printf("server accept failed...\n"); 
        exit(0); 
    } else {
        printf("server accept the client...\n");
    }
    */
    fd_set readfds = {};

    for (;;) {

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        int max_sd = sockfd;

        // Add client sockets to read set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients_fd[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        // Wait for an activity
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }

        // New connection
        if (FD_ISSET(sockfd, &readfds)) {
            struct sockaddr_in  address = {}; 
            socklen_t addrlen = sizeof(address);
            int new_socket = accept(sockfd, (struct sockaddr*)&address, &addrlen);
            if (new_socket < 0) {
                perror("Accept error");
                continue;
            }

            printf("New connection: socket %d\n", new_socket);

            // Add new socket to client list
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients_fd[i] == 0) {
                    clients_fd[i] = new_socket;
                    break;
                }
            }
        }

        char buff[128] = {};
        // Handle client messages
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients_fd[i];
            if (FD_ISSET(sd, &readfds)) {
                int bytes_read = read(sd, buff, sizeof(buff));
                if (bytes_read == 1) {
                    printf("Client disconnected: socket %d\n", sd);
                    fflush(NULL);
                    close(sd);
                    clients_fd[i] = 0;
                } else {
                    buff[bytes_read] = '\0';
                    printf("Received: %s\n", buff);
                    fflush(NULL);
                    send(sd, buff, bytes_read, 0);  // Echo message back
                }
            }
        }

        /*
        char buff[128] = {};
        ssize_t bytes_len = read(connfd, buff, sizeof(buff));

        if (bytes_len != -1) {
            
            char * text = "perae boy foi leigo agr\n";
            write(connfd, text, strlen(text));

            printf("error boy, linha %s:%d\n", __FILE__, __LINE__);
            //exit(-1);
        }
        */
    }

    close(sockfd);
}
