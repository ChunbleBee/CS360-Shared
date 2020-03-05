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

#include <arpa/inet.h>

#define LINEMAX 256

char * permAvailable =  "xwrxwrxwr-------";
char * permRestricted = "----------------";

struct hostent * host_entry;

struct sockaddr_in server_addr, client_addr, name_addr;

int server_socket, client_socket, server_port;

char cwd[4096];
char line[LINEMAX + 1];

void lsFile(char * fileStr);
void lsDir(char * dirStr);

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
        char buffer[LINEMAX];
        int fdesc = open(file, O_RDONLY);

        if (fdesc != -1) {
            while (read(fdesc, &buffer, LINEMAX) > 0) {
                write(client_socket, buffer, LINEMAX);
                bzero(buffer, LINEMAX);
            }
            write(client_socket, "\n", LINEMAX);
        } else {
            write(client_socket, "Error: could not open file [ ", LINEMAX);
            write(client_socket, file, LINEMAX);
            write(client_socket, " ] for reading.\n", LINEMAX);
        }
    } else {
        write(client_socket, "Error: could not open file [ ", LINEMAX);
        write(client_socket, file, LINEMAX);
        write(client_socket, " ] for reading.\n", LINEMAX);
    }
}

void ls(char * line) {
    char lsarg[256];
    strcpy(lsarg, line);
    strtok(lsarg, " ");
    char * paramStr = strtok(NULL, " ");
    
    if (paramStr != NULL) {
        strncpy(lsarg, paramStr, LINEMAX);
        struct stat * stats = (struct stat *) malloc(sizeof(struct stat));
        write(client_socket, "Permissions Links Group Owner Size Date Name\n", 46);

        if (lstat(lsarg, stats) == 0) {
            if (S_ISDIR(stats->st_mode)) {
                lsDir(lsarg);
            } else {
                lsFile(lsarg);
            }
        } else {
            write(client_socket, "Error: no such file or directory [ ", LINEMAX);
            write(client_socket, lsarg, LINEMAX);
            write(client_socket, " ] found.\n", LINEMAX);
        }

        free(stats);
    } else {
        write(client_socket, "Permissions Links Group Owner Size Date Name\n", LINEMAX);
        lsDir("./");
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
            strcat(path, dirStr);
            strcat(path, treebeard->d_name);
            lsFile(path);
            treebeard = readdir(dir);
        }
    } else {
        write(client_socket, "Error: no such directory [ ", LINEMAX);
        write(client_socket, dirStr, LINEMAX);
        write(client_socket, " ] found.\n", LINEMAX);
    }
}

void lsFile(char * fileStr) {
    if (access(fileStr, F_OK) == 0) {
        struct stat * stats = (struct stat *) malloc(sizeof(struct stat));
        lstat(fileStr, stats);

        char permissions[10];
        permissions[0] = 'd'; //Other/unknown type

        if (S_ISDIR(stats->st_mode)) {
            permissions[0] = 'd';
        } else if (S_ISREG(stats->st_mode)) {
            permissions[0] = '-';
        } /*else if (S_ISLINK(stats->st_mode)) {
            permissions[0] = 'l';
        }*/

        for (int i = 0; i < 8; i++) {
            if (stats->st_mode & (1 << i)) { // print r | w | x
                permissions[i+1] = permAvailable[i];
            } else {
                permissions[i+1] = permRestricted[i];
            }
        }

        char * fileTime = ctime(&(stats->st_ctime));
        fileTime[strlen(fileTime) - 1] = '\0';
        bzero(line, LINEMAX);

        sprintf(
            line,
            "%s %ld %d %d %ld %s %s\n",
            permissions,
            stats->st_nlink,
            stats->st_gid,
            stats->st_uid,
            stats->st_size,
            fileTime,
            fileStr + 2
        );
        write(client_socket, line, LINEMAX);

        free(stats);
    } else {
        write(client_socket, "Error: no such file [ ", LINEMAX);
        write(client_socket, fileStr, LINEMAX);
        write(client_socket, " ] found.\n", LINEMAX);
    }
}


int main (int argc, char * argv[], char * env[]) {
    char hostname[256];
    int n;

    if (argc < 2) {
        strcpy(hostname, "localhost");
        // gethostname(hostname, 256);
    }
    else
        strncpy(hostname, argv[1], 255);

    server_init(hostname);

    getcwd(cwd, 4096);
    int changed = chroot(cwd);
    printf("%i %s\n", changed, cwd);

    if (changed != 0) {
        printf("error: chroot failed\n");
        exit(8);
    }
    chdir("/");
    getcwd(cwd, 4096);
    printf("server: changed root to current directory\n");
    if (setgid(getgid()) == -1) {
        printf("error: failed to release permissions\n");
        exit(9);
    }
    if (setuid(getuid()) == -1) {
        printf("error: failed to release permissions\n");
        exit(10);
    }
    printf("server: released root privileges\n");

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

                if (strncmp(line, "pwd", 3) == 0) {
                    strncpy(line, cwd, LINEMAX);
                    printf("sending: %s\n", line);
                    write(client_socket, line, LINEMAX);
                } else if (strncmp(line, "ls", 2) == 0) {
                    ls(line);
                } else if (strncmp(line, "cat", 3) == 0) {
                    cat(line);
                } else if (!strncmp(line, "cd", 2)) {
                    strtok(line, " ");
                    char * dirpath = strtok(NULL, " ");
                    if (chdir(dirpath) != 0)
                        write(client_socket, "error: could not find directory\n", LINEMAX);
                    getcwd(cwd, 4096);
                } else if (strncmp(line, "mkdir", 5) == 0) {
                    strtok(line, " ");
                    char * name = strtok(NULL, " ");
                    printf("\t%s %s\n", name, "0755");

                    if (
                        name != NULL &&
                        opendir(name) == NULL
                    ) {
                        mkdir(name, 0755);
    
                        write(client_socket, "Created directory [ ", LINEMAX);
                        write(client_socket, name, LINEMAX);
                        write(client_socket, " ].\n", LINEMAX);
                    }  else {
                        write(client_socket, "Error: could not create directory [ ", LINEMAX);
                        write(client_socket, name, LINEMAX);
                        write(client_socket, " ].\n", LINEMAX);
                    }
                } else if (!strncmp(line, "rmdir", 5)) {
                    strtok(line, " ");
                    char * name = strtok(NULL, " ");

                    if (
                        name != NULL &&
                        opendir(name) != NULL
                    ) {
                        rmdir(name);

                        write(client_socket, "Successfully removed directory [ ", LINEMAX);
                        write(client_socket, name, LINEMAX);
                        write(client_socket, " ].\n", LINEMAX);
                    } else {
                        write(client_socket, "Error: could not remove directory [ ", LINEMAX);
                        write(client_socket, name, LINEMAX);
                        write(client_socket, " ].\n", LINEMAX);
                    }
                } else if (!strncmp(line, "rm", 2)) {
                    strtok(line, " ");
                    char * name = strtok(NULL, " ");

                    if (
                        name != NULL &&
                        access(name, F_OK) == 0
                    ) {
                        remove(name);

                        write(client_socket, "Successfully removed filed [ ", LINEMAX);
                        write(client_socket, name, LINEMAX);
                        write(client_socket, " ].\n", LINEMAX);
                    } else {
                        write(client_socket, "Error: could not remove file [ ", LINEMAX);
                        write(client_socket, name, LINEMAX);
                        write(client_socket, " ].\n", LINEMAX);
                    }
                } else if (!strncmp(line, "get", 3)) {
                    write(client_socket, "Server: TODO -- get\n", LINEMAX);
                } else if (!strncmp(line, "put", 3)) {
                    write(client_socket, "Server: TODO -- put\n", LINEMAX);
                } else {
                    strcpy(line, "server: command not found\n");
                    write(client_socket, line, LINEMAX);
                }
                write(client_socket, "", LINEMAX);
            }
        }
    }
}