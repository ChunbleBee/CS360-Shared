
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <stddef.h>

#include "type.h"

//////// global variables ////////
MINODE minode[NMINODE];
MINODE *root;

PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start; // disk parameters

char linkedNameBuf[60]; // global buffer for readlink
//////////////////////////////////

#include "util.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"

int init()
{
    int i, j;
    MINODE *mip;
    PROC   *p;

    printf("init()\n");

    for (i=0; i<NMINODE; i++) {
        mip = &minode[i];
        mip->dev = mip->ino = 0;
        mip->refCount = 0;
        mip->mounted = 0;
        mip->mptr = 0;
    }
    for (i=0; i<NPROC; i++) {
        p = &proc[i];
        p->pid = i;
        p->uid = p->gid = 0;
        p->cwd = 0;
        p->status = FREE;
        for (j=0; j<NFD; j++)
            p->fd[j] = 0;
    }
}

// load root INODE and set root pointer to it
int mount_root() {  
    printf("mount_root()\n");
    root = iget(dev, 2);
}

int quit() {
    int i;
    MINODE *mip;
    for (i=0; i<NMINODE; i++) {
        mip = &minode[i];
        while (mip->refCount > 0)
        iput(mip);
    }
    exit(0);
}

char *disk = "diskimage";
int main(int argc, char *argv[ ]) {
    int ino;
    char spbuf[BLKSIZE], gpbuf[BLKSIZE];
    char line[128], cmd[32], pathname[128], pathname2[128];
  
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

    printf("creating P0 as running process\n");
    running = &proc[0];
    running->status = READY;
    running->cwd = iget(dev, 2);
    printf("root refCount = %d\n", root->refCount);

    // WRTIE code here to create P1 as a USER process
    
    while(1) {
        printf("input command : [ ls | cd | pwd | mkdir | creat | rmdir\n");
        printf("                | link | unlink | symlink | readlink | quit] ");
        fgets(line, 128, stdin);
        line[strlen(line)-1] = '\0';

        if (line[0] == '\0')
            continue;
        pathname[0] = '\0';
        pathname2[0] = '\0';

        sscanf(line, "%s %s %s", cmd, pathname, pathname2);
        printf("cmd=%s pathname=%s pathname2=%s\n", cmd, pathname, pathname2);
      
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
        } else printf("no command, cmd: %s", cmd);
    }
}
