#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define LINEMAX 256

struct hostent * host_entry;

struct sockaddr_in server_addr, client_addr, name_addr;

int server_socket, client_socket, server_port;

void server_init(char * name) {
    printf("Initializing server\n");
    host_entry = gethostbyname(name);
    if (host_entry == 0) {
        printf("unknown host\n");
        exit(1);
    }
    printf("Server host info:\n");
    printf("    hostname=%s  ", host_entry->h_name);
    printf("IP=%s\n", inet_ntoa(*((struct in_addr *) host_entry->h_addr_list[0])));
    printf("Creating socket\n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("failed to create socket\n");
        exit(2);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = 0;
    printf("Assigning name to socket\n");
    if (bind(server_socket, (struct sockaddr *) & server_addr,
            sizeof(server_addr)) != 0) {
        printf("failed to bind socket to address");
        exit(3);
    }
    printf("Getting port number from kernel\n");
    int len_name_addr = sizeof(name_addr);
    if (getsockname(server_socket, (struct sockaddr *) & name_addr,
            & len_name_addr) != 0) {
        printf("failed getting socket name\n");
        exit(4);
    }
    server_port = ntohs(name_addr.sin_port);
    printf("    port=%d\n", server_port);
    listen(server_socket, 4);
    printf("Server Initialized\n");
}

int main (int argc, char * argv[], char * env[]) {
    char hostname[256];
    char line[LINEMAX];
    int n;

    if (argc < 2)
        gethostname(hostname, 256);
    else
        strncpy(hostname, argv[1], 255);

    server_init(hostname);

    while (1) {
        printf("server: accepting new connection ...\n");
        int len_client_addr = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) & client_addr,
            & len_client_addr);
        if (client_socket < 0) {
            printf("server: error accepting new client\n");
            exit(5);
        }
        
        printf("server: accepteda client:\n");
        printf("    IP=%s  port=%d\n", inet_ntoa(client_addr.sin_addr.s_addr),
                ntohs(client_addr.sin_port));
        if (fork()) { // parent
            close(client_socket);
        }
        else {
            while (1) { // processing loop
                printf("server: waiting for request from client ...\n");
                n = read(client_socket, line, LINEMAX);
                if (n == 0) {
                    printf("server: client disconnected\n");
                    close(client_socket);
                    exit(0);
                }
                printf("server: read n=%d bytes:\n    %s\n", n, line);

                strcat(line, " ECHO"); // TODO: remove this line!!!

                n = write(client_socket, line, LINEMAX);

                printf("server: wrote n=%d bytes:\n    %s\n", n, line);
            }
        }
    }
}