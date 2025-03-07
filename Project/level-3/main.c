#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <stddef.h>
#include <math.h>

#include "type.h"

//////// global variables ////////
MINODE minode[NMINODE];
MINODE *root;

PROC   proc[NPROC], *running;

OFT oft[NOFT];

MTABLE mtable[NMTABLE];

char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

char spbuf[BLKSIZE], gpbuf[BLKSIZE];

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start; // disk parameters

char linkedNameBuffer[60]; // global buffer for readlink

char * disk = "mydisk";
//////////////////////////////////

#include "util.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
#include "open_close.c"
#include "read_cat.c"
#include "write_cp.c"
#include "mount_umount.c"

int init()
{
    int i, j;
    MINODE *mip;
    PROC   *p;
    OFT * p_oft;
    MTABLE * p_mtable;

    printf("init()\n");

    for (i=0; i<NMINODE; i++) {
        mip = &minode[i];
        mip->dev = mip->ino = 0;
        mip->refCount = 0;
        mip->mounted = 0;
        mip->mode = -1;
        mip->mptr = NULL;
    }
    for (i=0; i<NPROC; i++) {
        p = &proc[i];
        p->pid = i;
        p->uid = p->gid = 0;
        p->cwd = NULL;
        p->status = FREE;
        for (j=0; j<NFD; j++)
            p->fd[j] = NULL;
    }
    for (i=0; i<NOFT; i++) {
        p_oft = &oft[i];
        p_oft->mode = -1;
        p_oft->mptr = NULL;
        p_oft->offset = 0;
        p_oft->refCount = 0;
    }
    for (i=0; i<NMTABLE; i++) {
        p_mtable = &mtable[i];
        p_mtable->dev = 0;
        p_mtable->mptr = NULL;
        p_mtable->name[0] = '\0';
        p_mtable->ninodes = 0;
        p_mtable->nblocks = 0;
        p_mtable->bmap = 0;
        p_mtable->imap = 0;
        p_mtable->inode_start = 0;
    }
}

int create_process(u32 usertype) {
    if (usertype >= running->gid) {
        for (int i = 0; i < NPROC; i++) {
            if (proc[i].status == FREE) {
                proc[i].status = READY;
                proc[i].uid = i;
                proc[i].gid = usertype;
                proc[i].cwd = root;
                return proc[i].pid;
            }
        }
        printf("The kernel panics... for all processes are in use.\n");
    } else {
        printf("Cannot create a new process with a higher user-permissions level than current.\n");
    }
    return -1;
}

int switch_process(u32 pid) {
    if (pid >= 0 && pid < NPROC && proc[pid].status == READY) {
        running = &proc[pid];
        printf("Successfully switched to process %u\n", running->pid);
        return 1;
    }
    printf("Invalid process ID\n");
    return 0;
}

int kill_process(u32 pid) {
    if (pid > 0 && pid < NPROC && proc[pid].status == READY) {
        if (proc[pid].gid < running->gid || proc[pid].uid == running->uid) {
            int isCurrent = (proc[pid].uid == running->uid) ? 1 : 0;
            for(int i = 0; i < NFD; i++) {
                if (proc[pid].fd[i] != NULL) {
                    close_file(i);
                }
                proc[pid].fd[i] = NULL;
            }
            proc[pid].status = FREE;

            if (isCurrent) {
                running = &proc[0];
            }
        } else {
            printf("Incorrect permissions to kill this process!\n");
            return -1;
        }
    }
    printf("Invalid Process id.\n");
    return -1;
}

// load root INODE and set root pointer to it
int mount_root() {  
    printf("mount_root()\n");
    root = iget(dev, 2);
}

int quit() {
    if (running->pid != 0)
        switch_process(0);
    int i;
    MINODE *mip;
    for (i = 0; i < NMINODE; i++) {
        mip = &minode[i];
        while (mip->refCount > 0) {
            iput(mip);
        }
    }
    MTABLE *mtp;
    for (i=0; i<NMTABLE; i++) {
        mtp = &mtable[i];
        if (mtp->dev > 0) {
            close(mtp->dev);
        }
    }
    // currently only writes back to root device
    put_block(root->dev, 1, spbuf);
    put_block(root->dev, 2, gpbuf);
    close(fd);
    exit(0);
}

int main(int argc, char *argv[ ]) {
    int ino;
    char line[128], cmd[32], pathname[128], pathname2[128];
    char * writebuffer;

    if (argc > 1) {
        disk = argv[1];
    }
  
    printf("checking EXT2 FS ....");
    if ((fd = open(disk, O_RDWR)) < 0) {
        printf("open %s failed\n", disk);
        exit(1);
    }
    dev = fd;    // fd is the global dev

    /********** read super block  ****************/
    get_block(dev, 1, spbuf);
    sp = (SUPER *)spbuf;

    /* verify it's an ext2 file system ***********/
    if (sp->s_magic != 0xEF53) {
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        exit(1);
    }     
    printf("EXT2 FS OK\n");
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;

    get_block(dev, 2, gpbuf); 
    gp = (GD *)gpbuf;

    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    inode_start = gp->bg_inode_table;
    printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

    init();  
    mount_root();
    printf("root refCount = %d\n", root->refCount);

    printf("creating P0 as root, running process\n");
    running = &proc[0];
    running->status = READY;
    running->cwd = iget(dev, 2);
    printf("root refCount = %d\n", root->refCount);

    printf("Attempting to create user-group process...\n");
    int upid = create_process(USER_GROUP);
    if (upid != -1) {
        printf("Created user-level process #P%d\n", upid);
    } else {
        printf("Failed to create new user-level process.\n");
    }

    while(1) {
        printf("[ ls | cd | pwd | mkdir | creat | rmdir\n");
        printf("| link | unlink | symlink | readlink | quit |\n");
        printf("| cat | write | append | cp | mount | umount |\n");
        printf("| new | switch | kill ] $> ");
        fgets(line, 128, stdin);
        line[strlen(line)-1] = '\0';

        if (line[0] == '\0')
            continue;
        pathname[0] = '\0';
        pathname2[0] = '\0';

        sscanf(line, "%s %s %s", cmd, pathname, pathname2);
        

        printf("cmd=%s pathname=%s pathname2=%s line=%s\n",
            cmd, pathname, pathname2, line);
      
        if (strcmp(cmd, "ls") == 0)
            ls(pathname);
        else if (strcmp(cmd, "cd") == 0)
            chdir(pathname);
        else if (strcmp(cmd, "pwd") == 0)
            pwd(running->cwd);
        else if (strcmp(cmd, "quit") == 0)
            quit();
        else if (strcmp(cmd, "mkdir") == 0) {
            if (strcmp(pathname, "") != 0) {
                if (tryMakeDirectory(pathname) < 0) {
                    printf("mkdir %s failed\n", pathname);
                }
            } else {
                printf("Error: No path specified!\n");
            }
        } else if (strcmp(cmd, "creat") == 0) {
            if (tryCreate(pathname) < 0) {
                printf("creat %s failed\n", pathname);
            }
        } else if (strcmp(cmd, "rmdir") == 0) {
            if (tryRemoveDirectory(pathname) < 0) {
                printf("rmdir %s failed\n", pathname);
            }
        } else if (strcmp(cmd, "link") == 0) {
            if (tryLink(pathname, pathname2) < 0) {
                printf("link %s %s failed\n", pathname, pathname2);
            }
        } else if (strcmp(cmd, "unlink") == 0) {
            if (tryUnlink(pathname) < 0) {
                printf("unlink %s failed\n", pathname);
            }
        } else if (strcmp(cmd, "symlink") == 0) {
            if (symlink(pathname, pathname2) < 0) {
                printf("symlink %s %s failed\n", pathname, pathname2);
            }
        } else if (strcmp(cmd, "readlink") == 0) {
            if (readlinkFromPath(pathname) < 0) {
                printf("readlink %s failed\n", pathname);
            }
        } else if (strcmp(cmd, "cat") == 0) {
            cat(pathname);
        } else if (strcmp(cmd, "write") == 0) {
            int fileDesc = open_file(pathname, WRITE_MODE);
            strtok(line, " ");
            strtok(NULL, " ");
            writebuffer = strtok(NULL, "\n");
            printf("writebuffer: %s\n", writebuffer);
            tryWrite(fileDesc, writebuffer, strlen(writebuffer));
            close_file(fileDesc);
        }  else if (strcmp(cmd, "append") == 0) {
            int fileDesc = open_file(pathname, APPEND_MODE);
            strtok(line, " ");
            strtok(NULL, " ");
            writebuffer = strtok(NULL, "\n");
            printf("writebuffer: %s\n", writebuffer);
            tryWrite(fileDesc, writebuffer, strlen(writebuffer));
            close_file(fileDesc);
        } else if (strcmp(cmd, "cp") == 0) {
            tryCopy(pathname, pathname2);
        } else if (strcmp(cmd, "mount") == 0) {
            if (pathname[0] == '\0') {
                printMTables();
            } else if (pathname2[0] == 0) {
                printf("must provide mount point for mounting:\n");
                printf("    mount <diskname> <mountpoint>\n");
            } else {
                int err = mount(pathname, pathname2);
                if (err < 0) {
                    printf("mount failed\n");
                }
            }
        } else if (strcmp(cmd, "umount") == 0) {
            if (pathname[0] == '\0') {
                printf("mous provide diskimage to un-mount:\n");
                printf("    umount <diskname>\n");
            } else {
                int err = umount(pathname);
                if (err < 0) {
                    printf("umount failed\n");
                }
            }
        } else if (strcmp(cmd, "new") == 0) {
            u32 usertype = ERROR_GROUP;

            if (strcmp(pathname, "user") == 0 || strcmp(pathname, "") == 0) {
                usertype = USER_GROUP;
            } else if (strcmp(pathname, "other") == 0) {
                usertype = OTHER_GROUP;
            } else if (strcmp(pathname, "root") == 0) {
                usertype = ROOT_GROUP;
            }

            if (usertype != ERROR_GROUP) {
                int new = create_process(usertype);
                if (new != -1) {
                    printf("Created new process #p%d\n", new);
                } else {
                    printf("Failed to create a new process!\n");
                }
            } else {
                printf("Cannot create process of user type: [ %s ]!\n", pathname);
            }
        } else if (strcmp(cmd, "switch") == 0) {
            if (strlen(pathname) > 0) {
                u32 pid = atoi(&pathname[1]);
                switch_process(pid);
            } 
        } else if (strcmp(cmd, "kill") == 0) {
            if (strlen(pathname) > 0) {
                u32 pid = atoi(&pathname[1]);
                kill_process(pid);
            }
        } else printf("no command, cmd: %s", cmd);
    }
}
