#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>

#define PORT 1234

int sockfd = -1;

static void signal_handler(int sig){
    if (sig == SIGINT) {
        printf("Shutting down...\n");
        close(sockfd);
        exit(0);
    }
}

struct sigaction sigact = {};

void init_signal_handler(void){
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction*) NULL);
}

static _Bool is_ready_for_reading(int fd) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    return poll(&pfd, 1, 0) > 0;
}

int main () {

    int connfd, len; 
    struct sockaddr_in servaddr = {};
    struct sockaddr_in  cli = {}; 
  
    init_signal_handler();

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
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
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    } else {
        printf("connected to the server..\n");
    }

    for (;;) {
        char buff[128] = {};
        if (is_ready_for_reading(sockfd)) {
            ssize_t bytes_read = read(sockfd, buff, sizeof(buff));
            if (bytes_read > 0) {
                buff[bytes_read] = '\0';
                printf("read = %s\n", buff);
            }
        }
    }

    return 0;
}
