#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define LINEMAX 256

int mysocket, clientsocket;

int main (int argc, char * argv[], char * env[]) {
    char * hostname;
    char line[LINEMAX];

    if (argc < 2)
        hostname = "localhost";
    else
        hostname = argv[1];

    server_init(hostname);

    while (1) {
        printf("server: accepting new connection ...\n");

    }
}