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

struct hostent * host_entry;
struct sockaddr_in server_addr;

char * permAvailable =  "xwrxwrxwr-------";
char * permRestricted = "----------------";

struct in_addr server_ip;

int server_socket, server_port;

void lsFile(char * fileStr);
void lsDir(char * dirStr);
 
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
    printf("connected to \n");
    char ip[24];
    inet_ntop(AF_INET, (struct in_addr *) host_entry->h_addr_list[0], ip, sizeof(ip));
    printf("    hostname=%s IP=%s port=%d\n", host_entry->h_name,
            ip, server_port);
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
            close(fdesc);
        } else {
            printf("Error: could not open file [ %s ] for reading.\n", file);
        }
    } else {
        printf("Error: no such file [ %s ] was found.\n", file);
    }
}

void ls(char * line) {
    strtok(line, " ");
    char * paramStr = strtok(NULL, " ");

    if (paramStr != NULL) {
        struct stat * stats = (struct stat *) malloc(sizeof(struct stat));
        if (lstat(paramStr, stats) == 0) {
            printf("Permissions Links Group Owner Size Date Name\n");
            if (S_ISDIR(stats->st_mode)) {
                lsDir(paramStr);
            } else {
                lsFile(paramStr);
            }
        } else {
            printf("Error: no such file or directory [ %s ] found.\n", paramStr);
        }
        free(stats);
    } else {
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
        closedir(dir);
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
        for (int i = 0; i < 8; i++) {
            if (stats->st_mode & (1 << i)) { // print r | w | x
                printf("%c", permAvailable[i]);
            } else {
                printf("%c", permRestricted[i]);
            }
        }

        char * fileTime = ctime(&(stats->st_ctime));
        fileTime[strlen(fileTime) - 1] = '\0';
        
        // Permissions Links Group Owner Size Date Name
        printf(" %ld", stats->st_nlink);
        printf(" %d", stats->st_gid);
        printf(" %d", stats->st_uid);
        printf(" %ld", stats->st_size);
        printf(" %s", fileTime);
        printf(" %s\n", fileStr + 2);

        free(stats);
    } else {
        printf("Error: no such file [ %s ] found.\n", fileStr);
    }
}

void put(char * line) {
    char linecpy[LINEMAX + 1];
    strcpy(linecpy, line);
    strtok(linecpy, " ");
    char * file = strtok(NULL, " ");

    if (access(file, F_OK) >= 0) {
        int fdesc = open(file, O_RDONLY);

        if (fdesc != -1) {
            struct stat fstat;
            stat(file, &fstat);
            long length = fstat.st_size;

            write(server_socket, line, LINEMAX);
            read(server_socket, line, LINEMAX);
            printf("server: %s", line);
            if (strncmp(line, "error", 5) == 0) {
                close(fdesc);
                return;
            }

            write(server_socket, &(fstat.st_size), sizeof(long));
            printf("sending total file length: %ld bytes\n", length);
            int n;
            while (length > LINEMAX) {
                n = read(fdesc, line, LINEMAX);
                length -= n;
                write(server_socket, line, LINEMAX);
                printf("wrote n=%d bytes to client, remaining length=%ld\n",
                        n, length);
            }
            n = read(fdesc, line, LINEMAX);
            length -= n;
            write(server_socket, line, n);
            printf("wrote n=%d bytes to client, remaining length=%ld\n",
                    n, length);
            close(fdesc);
        } else {
            printf("Error: could not open file [ %s ] for reading.\n", file);
        }
    } else {
        printf("Error: could not open file [ %s ] for reading.\n", file);
    }
}


int main (int argc, char * argv[], char * env[]) {
    int n;
    char line[LINEMAX + 1];

    if (argc < 3) {
        printf("Required:\n    cient.bin <<ServerName>> <<ServerPort>>\n");
        exit(1);
    }

    client_init(argv);
    while (true) {
        printf("| get  put  cat  ls  cd  pwd  mkdir  rmdir  rm |\n");
        printf("|          lcat lls lcd lpwd lmkdir lrmdir lrm |\n");
        printf("input a line : ");
        bzero(line, LINEMAX);                // zero out line[ ]
        fgets(line, LINEMAX, stdin);         // get a line (end with \n) from stdin

        line[strlen(line) - 1] = '\0';

        if (strncmp(line, "quit", 4) == 0){
            write(server_socket, line, LINEMAX);
            exit(0);
        } else if (strncmp(line, "lcat", 3) == 0) {
            cat(line);
        } else if (strncmp(line, "lpwd", 4) == 0) {
            char buffer[512];
            getcwd(buffer, 512);
            printf("%s\n", buffer);
        } else if (strncmp(line, "lls", 3) == 0) {
            ls(line);
        } else if (strncmp(line, "lcd", 3) == 0) {
            strtok(line, " ");
            char * dirpath = strtok(NULL, " ");
            if (chdir(dirpath) != 0)
                printf("error: could not find directory\n");
        } else if (strncmp(line, "lmkdir", 6) == 0) {
            strtok(line, " ");
            char * name = strtok(NULL, " ");

            if (
                name != NULL &&
                opendir(name) == NULL
            ) {
                mkdir(name, 0755);
            }  else {
                printf("Error: could not create directory [ %s ].\n", name);
            }
        } else if (strncmp(line, "lrmdir", 6) == 0) {
            strtok(line, " ");
            char * name = strtok(NULL, " ");

            if (
                name != NULL &&
                opendir(name) != NULL
            ) {
                rmdir(name);
            } else {
                printf("Error: could not remove directory [ %s ].\n", name);
            }
        } else if (strncmp(line, "lrm", 3) == 0) {
            strtok(line, " ");
            char * name = strtok(NULL, " ");

            if (
                name != NULL &&
                access(name, F_OK) == 0
            ) {
                remove(name);
            } else {
                printf("Error: could not remove file [ %s ].\n", name);
            }
        } else {
            // Send ENTIRE line to server
            n = write(server_socket, line, LINEMAX);
            printf("client: wrote n=%d bytes:\n    %s\n", n, line);

            if (
                strncmp(line, "pwd", 3) == 0 ||
                strncmp(line, "ls", 2) == 0 ||
                strncmp(line, "mkdir", 5) == 0 ||
                strncmp(line, "rmdir", 5) == 0 ||
                strncmp(line, "rm", 2) == 0 ||
                strncmp(line, "cd", 2) == 0) {
                printf("\tServer Response:\n\n");
                bzero(line, LINEMAX);
                read(server_socket, line, LINEMAX);
                while (strcmp(line, "") != 0) {
                    printf("%s", line);
                    bzero(line, LINEMAX);
                    read(server_socket, line, LINEMAX);
                }
                printf("\n");
            } else if (strncmp(line, "cat", 3) == 0) {
                read(server_socket, line, LINEMAX);
                
                printf("\tServer Response:\n    %s", line);
                if (strncmp(line, "Error", 5) == 0) {
                    read(server_socket, line, LINEMAX);
                    printf("%s", line);
                    read(server_socket, line, LINEMAX);
                    printf("%s\n", line);
                } else {
                    printf("\n");
                    bzero(line, LINEMAX + 1);
                    long length = 0;
                    read(server_socket, &length, sizeof(long));
                    printf("Total File Length: %ld bytes:\n\n", length);
                    int n;
                    while (length > LINEMAX) {
                        n = read(server_socket, line, LINEMAX);
                        length -= n;
                        printf("%s", line);
                    }
                    n = read(server_socket, line, length);
                    line[n] = '\0';
                    printf("%s\n\nfinished transmission\n", line);
                }
                read(server_socket, line, LINEMAX);
            } else if (strncmp(line, "get", 3) == 0) {
                char linecpy[LINEMAX + 1];
                strcpy(linecpy, line);
                read(server_socket, line, LINEMAX);
                
                printf("\tServer Response:\n    %s", line);
                if (strncmp(line, "Error", 5) == 0) {
                    read(server_socket, line, LINEMAX);
                    printf("%s", line);
                    read(server_socket, line, LINEMAX);
                    printf("%s\n", line);
                } else {
                    printf("\n");
                    bzero(line, LINEMAX + 1);
                    long length = 0;
                    read(server_socket, &length, sizeof(long));
                    printf("Total File Length: %ld bytes:\n\n", length);
                    int n;
                    strtok(linecpy, " ");
                    char * filename = strtok(NULL, " ");
                    int fd = open(filename, O_WRONLY|O_CREAT, 0644);
                    while (length > LINEMAX) {
                        n = read(server_socket, line, LINEMAX);
                        length -= n;
                        write(fd, line, LINEMAX);
                        printf("wrote n=%d bytes to file=%s, remaining length=%ld\n",
                                n, filename, length);
                    }
                    n = read(server_socket, line, length);
                    length -+ n;
                    write(fd, line, n);
                    printf("wrote n=%d bytes to file=%s, remaining length=%ld\n",
                            n, filename, length);
                    close(fd);
                }
                read(server_socket, line, LINEMAX);
            } else if (strncmp(line, "put", 3) == 0) {
                put(line);
                read(server_socket, line, LINEMAX);
            } else {
                read(server_socket, line, LINEMAX);
                printf("server:\n    %s\n", line);
                read(server_socket, line, LINEMAX);
            }
        }
    }
}