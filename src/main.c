#define _XOPEN_SOURCE 700
#include <string.h>
#include <stdio.h>
#include "client.h"
#include "server.h"


int main(int argc, char ** argv) {

    if (argc != 2) {
        puts("Usage: pass '--client' or '--server'.");
        return 1;
    }

    if (strcmp(argv[1], "--client") == 0) {
        puts("Running in client mode...");

        Client client = {};
        init_client(&client, CM_MULTIPLAYER);
        run_client(&client);

    } else if (strcmp(argv[1], "--server") == 0) {
        puts("Running in server mode...");

        Server server = {};
        init_server(&server);
        run_server(&server);

    } else {
        puts("Invalid option. Use '--client' or '--server'.");
        return 1;
    }

    return 0;
}
