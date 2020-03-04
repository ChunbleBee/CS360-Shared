#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#define LINEMAX 256

struct hostent * host_entry;
struct sockaddr_in server_addr;

struct in_addr server_ip;

int server_socket, server_port;

void client_init(char * argv[]) {
    printf("Initializing client\n");
    host_entry = gethostbyname(argv[1]);
    if (host_entry == NULL) {
        printf("unknown host %s\n", argv[1]);
        exit(2);
    }

    server_ip = *((struct in_addr *) host_entry->h_addr_list[0]);
    server_port = atoi(argv[2]);

    printf("Creating TCP socket\n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("failed to create socket\n");
        exit(3);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = server_ip.s_addr;
    server_addr.sin_port = htons(server_port);
    printf("Connecting to server\n");
    if (connect(server_socket, (struct sockaddr *) & server_addr,
            sizeof(server_addr)) < 0) {
        printf("connection failed\n");
        exit(4);
    }
    printf("connected to \007\n");
    printf("    hostname=%s IP=%s port=%d\n", host_entry->h_name,
            inet_ntoa(server_ip.s_addr), server_port);
}

int main (int argc, char * argv[], char * env[]) {
    int n;
    char line[LINEMAX];

    if (argc < 3) {
        printf("Required:\n    cient.bin <<ServerName>> <<ServerPort>>\n");
        exit(1);
    }

    client_init(argv);

    while (strncmp(line, "quit", 4)) {
        printf("input a line:\n$ ");
        bzero(line, LINEMAX);
        fgets(line, LINEMAX, stdin);

        line[strlen(line) - 1] = '\0';
        if (line[0] != '\0') {
            n = write(server_socket, line, LINEMAX);
            printf("client: wrote n=%d bytes:\n    %s\n", n, line);
            n = read(server_socket, line, LINEMAX);
            printf("client: read n=%d bytes:\n    %s\n", n, line);
        }
    }
}