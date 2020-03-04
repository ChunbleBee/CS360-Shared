#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>

#define LINEMAX 256

char * permAvailable =  "xwrxwrxwr-------";
char * permRestricted = "----------------";

struct hostent * host_entry;

struct sockaddr_in server_addr, client_addr, name_addr;

int server_socket, client_socket, server_port;

char cwd[4096];
char line[LINEMAX + 1];

void server_init(char * name) {
    printf("Initializing server\n");
    host_entry = gethostbyname(name);
    if (host_entry == NULL) {
        printf("unknown host\n");
        exit(1);
    }
    printf("Server host info:\n");
    printf("    hostname=%s\n", name);
    char ip[16];
    inet_ntop(AF_INET, (struct in_addr *) host_entry->h_addr_list[0], ip, sizeof(ip));
    printf("    IP=%s\n", ip);
    // printf("IP=%s\n", inet_ntoa(*((struct in_addr *) host_entry->h_addr_list[0])));
    printf("Creating socket\n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("failed to create socket\n");
        exit(2);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = *((struct in_addr *) host_entry->h_addr_list[0]);
    // server_addr.sin_addr.s_addr = *(long *) host_entry->h_addr_list[0];
    server_addr.sin_port = 0; // kernal will assign port number
    printf("Assigning name to socket\n");
    if (bind(server_socket, (struct sockaddr *) & server_addr,
            sizeof(server_addr)) != 0) {
        printf("failed to bind socket to address\n");
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
    listen(server_socket, 5);
    printf("Server Initialized\n");
}
 
void cat(char * line) {
    strtok(line, " ");
    char * file = strtok(NULL, " ");

    if (access(file, F_OK) >= 0) {
        char buffer[1024];
        int fdesc = open(file, O_RDONLY);

        if (fdesc != -1) {
            while (read(fdesc, &buffer, 1024) > 0) {
                printf("%s", buffer);
                bzero(buffer, 1024);
            }
            printf("\n");
        } else {
            printf("Error: could not open file [ %s ] for reading.\n", file);
        }
    } else {
        printf("Error: no such file [ %s ] was found.\n", file);
    }
}

void ls() {
    char lsarg[256];
    strtok(line, " ");
    char * paramStr = strtok(NULL, " ");

    if (paramStr != NULL) {
        strncpy(lsarg, paramStr, LINEMAX);
        struct stat * stats = (struct stat *) malloc(sizeof(struct stat));
        if (lstat(lsarg, stats) == 0) {
            strcpy(line, "Permissions Links Group Owner Size Date Name\n");
            write(client_socket, line, LINEMAX);
            if (S_ISDIR(stats->st_mode)) {
                lsDir(lsarg);
            } else {
                lsFile(lsarg);
            }
        } else {
            printf("Error: no such file or directory [ %s ] found.\n", lsarg);
        }
        free(stats);
    } else {
        lsDir(cwd);
    }
}


void lsDir(char * dirStr) {
    DIR * dir = opendir(dirStr);

    if (dir != NULL) {
        struct dirent * treebeard = readdir(dir);
        // "The world is changing:
        // I feel it in the water,
        // I feel it in the earth,
        // and I smell it in the air."
        // Treebeard, The Two Towers, J. R. R. Tolkien.

        char path[4356]; //max path length + entry name size = 4096 + 260 = 4356 characters
        while(treebeard != NULL) {
            bzero(path, 4356);
            strcat(&path, dirStr);
            strcat(&path, treebeard->d_name);
            lsFile(&path);
            treebeard = readdir(dir);
        }
    } else {
        printf("Error: no such directory [ %s ] found.\n", dirStr);
    }
}

void lsFile(char * fileStr) {
    if (access(fileStr, F_OK) == 0) {
        struct stat * stats = (struct stat *) malloc(sizeof(struct stat));
        lstat(fileStr, stats);

        char type = '0'; //Other/unknown type

        if (S_ISDIR(stats->st_mode)) {
            type = 'd';
        } else if (S_ISREG(stats->st_mode)) {
            type = '-';
        } /*else if (S_ISLINK(stats->st_mode)) {
            type = 'l';
        }*/

        printf("%c", type);
        char perm;
        for (int i = 0; i < 8; i++) {
            if (stats->st_mode & (1 << i)) { // print r | w | x
                perm = permAvailable[i];
            } else {
                perm = permRestricted[i];
            }
        }

        char * filetime = ctime(&(stats->st_ctime));
        filetime[strlen(filetime) - 1] = "\0";

        sprintf(line, "%c%c %d %d %d %d %s %s\n", type, perm, stats->st_nlink,
            stats->st_gid, stats->st_uid, stats->st_size, filetime, fileStr);

        free(stats);
    } else {
        printf("Error: no such file [ %s ] found.\n", fileStr);
    }
}


int main (int argc, char * argv[], char * env[]) {
    char hostname[256];
    int n;

    getcwd(cwd, 4096);
    if (chroot(cwd) != 0) {
        printf("error: chroot failed\n");
        exit(8);
    }
    strcpy(cwd, "/");
    printf("server: changed root to current directory\n");

    if (argc < 2) {
        strcpy(hostname, "localhost");
        // gethostname(hostname, 256);
    }
    else
        strncpy(hostname, argv[1], 255);

    server_init(hostname);

    while (true) {
        printf("server: accepting new connections . . .\n");
        int len_client_addr = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) & client_addr,
            & len_client_addr);
        if (client_socket < 0) {
            printf("server: error accepting new client\n");
            exit(5);
        }
        
        printf("server: accepted a client:\n");
        char ip[24];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("    IP=%s  port=%d\n", ip, ntohs(client_addr.sin_port));
        if (fork()) { // parent
            close(client_socket);
            printf("in parent process\n");
        }
        else {
            while (true) { // processing loop
                printf("server: waiting for request from client . . .\n");
                n = read(client_socket, line, LINEMAX);
                if (n == 0) {
                    printf("server: client disconnected\n");
                    close(client_socket);
                    exit(0);
                }
                printf("server: read n=%d bytes:\n    %s\n", n, line);

                if (!strncmp(line, "pwd", 3)) {
                    strncpy(line, cwd, LINEMAX);
                    write(client_socket, line, LINEMAX);
                } else if (!strncmp(line, "ls", 2)) {
                    ls();
                } else if (!strncmp(line, "cd", 2)) {
                    printf("TODO\n");
                } else if (!strncmp(line, "mkdir", 5)) {
                    printf("TODO\n");
                } else if (!strncmp(line, "rmdir", 5)) {
                    printf("TODO\n");
                } else if (!strncmp(line, "rm", 2)) {
                    printf("TODO\n");
                } else if (!strncmp(line, "get", 3)) {
                    printf("TODO\n");
                } else if (!strncmp(line, "put", 3)) {
                    printf("TODO\n");
                } else if (!strncmp(line, "quit", 4)) {
                    printf("TODO\n");
                } else {
                    strcpy(line, "server: command not found\n");
                    write(client_socket, line, LINEMAX);
                }
            }
        }
    }
}