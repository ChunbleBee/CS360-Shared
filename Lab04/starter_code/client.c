// The echo client client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>

#include <sys/socket.h>
#include <netdb.h>

#define MAX 256

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr;

char * permAvailable =  "xwrxwrxwr-------";
char * permRestricted = "----------------";

int server_sock, r;
int SERVER_IP, SERVER_PORT; 


// clinet initialization code
int client_init(char *argv[])
{
    printf("======= clinet init ==========\n");

    printf("1 : get server info\n");
    hp = gethostbyname(argv[1]);
    if (hp==0){
        printf("unknown host %s\n", argv[1]);
        exit(1);
    }

    SERVER_IP   = *(long *)hp->h_addr;
    SERVER_PORT = atoi(argv[2]);

    printf("2 : create a TCP socket\n");
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock<0){
        printf("socket call failed\n");
        exit(2);
    }

    printf("3 : fill server_addr with server's IP and PORT#\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = SERVER_IP;
    server_addr.sin_port = htons(SERVER_PORT);

    // Connect to server
    printf("4 : connecting to server ....\n");
    r = connect(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
    if (r < 0){
        printf("connect failed\n");
        exit(1);
    }

    printf("5 : connected OK to \007\n"); 
    printf("---------------------------------------------------------\n");
    printf(
        "hostname=%s  IP=%s  PORT=%d\n",
        hp->h_name, inet_ntoa(SERVER_IP),
        SERVER_PORT
    );
    printf("---------------------------------------------------------\n");

    printf("========= init done ==========\n");
}

// Author: Taiya Williams
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

// Author: Taiya Williams
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

// Author: Taiya Williams
void lsDir(char * dirStr) {
    DIR * dir = opendir(dirStr);

    if (dir != NULL) {
        struct dirent * treebeard = readdir(dir);
        // "The world is changing:
        // I feel it in the water,
        // I feel it in the earth,
        // and I smell it in the air."
        // Treebeard, The Two Towers, J. R. R. Tolkien.

        char path[4356]; //max path length + entry size...
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

// Author: Taiya Williams
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

        char * filetime = ctime(&(stats->st_ctime));
        filetime[strlen(filetime) - 1] = "\0";
        
        // Permissions Links Group Owner Size Date Name
        printf(" %d", stats->st_nlink);
        printf(" %d", stats->st_gid);
        printf(" %d", stats->st_uid);
        printf(" %d", stats->st_size);
        printf(" %s", filetime);
        printf(" %s\n", fileStr);

        free(stats);
    } else {
        printf("Error: no such file [ %s ] found.\n", fileStr);
    }
}

main(int argc, char *argv[ ])
{
    int n;
    char line[MAX], ans[MAX];

    if (argc < 3){
        printf("Usage: client ServerName SeverPort\n");
        exit(1);
    }

    //client_init(argv);
    // sock <---> server
    printf("********  processing loop  *********\n");
    while (true) {
        printf("input a line : ");
        bzero(line, MAX);                // zero out line[ ]
        fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

        line[strlen(line) - 1] = NULL;     // kill \n at end

        if (strncmp(line, "quit", 4) == 0){           // exit if NULL line
            exit(0);
        } else if (strncmp(line, "lcat", 3) == 0) {
            cat(line);
        } else if (strncmp(line, "lpwd", 4) == 0) {
            char buffer[512];
            getcwd(&buffer, 512);
            printf("%s\n", buffer);
        } else if (strncmp(line, "lls", 3) == 0) {
            ls(line);
        } else if (strncmp(line, "lcd", 3) == 0) {
            //Change local working directory
        } else if (strncmp(line, "lmkdir", 6) == 0) {
            strtok(line, " ");
            char * name = strtok(NULL, " ");
            char * mode = strtok(NULL, " ");

            if (
                name != NULL &&
                opendir(name) == NULL
            ) {
                if (mode == NULL) {
                    mkdir(name, mode);
                } else {
                    mkdir(name, "755");
                }
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
            n = write(server_sock, line, MAX);
            printf("client: wrote n=%d bytes; line=(%s)\n", n, line);
            
            // Read a line from sock and show it
            n = read(server_sock, ans, MAX);
            printf("client: read  n=%d bytes; echo=(%s)\n",n, ans);
            
            if (strncmp(line, "get", 3) == 0) {

            } else if (strncmp(line, "put", 3) == 0) {

            }
        }

    }
}