#define READ_MODE 0
#define WRITE_MODE 1
#define READ_WRITE_MODE 2
#define APPEND_MODE 3

int open_file(char * filename, int mode) {
    if (mode < 0 || mode > 3) {
        prinf("Error: open mode %d is not defined\n", mode);
        return -10;
    }
    if (filename[0] == '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }
    int inodeNum = getino(filename);
    if (inodeNum == 0) {
        char filenameCopy[128];
        strcpy(filenameCopy, filename);
        creat(filenameCopy);
        inodeNum = getino(filename);
    }
    MINODE * mountedINode = iget(dev, ino);

    // check for regular file
    if (S_ISREG(mountedINode->INODE.i_mode)) {
        printf("%s is a regular file\n", filename);
    } else {
        printf("%s is not a regular file\n", filename);
        iput(mountedINode);
        return -1;
    }

    // checking permissions for mode
    if ((mode == READ_MODE) || (mode == READ_WRITE_MODE)) {
        if ( (running->uid == 0) || // super user
            ((mountedINode->INODE.i_mode & 0400) &&
                (running->uid == mountedINode->INODE.i_uid)) || // user read
            ((mountedINode->INODE.i_mode & 0040) &&
                (running->uid == mountedINode->INODE.i_gid)) || // group read
             (mountedINode->INODE.i_mode & 0004) // other read
        ) {
            printf("read permission for %s granted\n", filename);
        } else {
            printf("read permission for %s denied\n", filename);
            iput(mountedINode);
            return -2;
        }
    }
    if ((mode == WRITE_MODE) || (mode == READ_WRITE_MODE) || (mode == APPEND_MODE)) {
        if ( (running->uid == 0) || // super user
            ((mountedINode->INODE.i_mode & 0200) &&
                (running->uid == mountedINode->INODE.i_uid)) || // user write
            ((mountedINode->INODE.i_mode & 0020) &&
                (running->uid == mountedINode->INODE.i_gid)) || // group write
             (mountedINode->INODE-i_mode & 0002) // other write
        ) {
            printf("write permission for %s granted\n", filename);
        } else {
            printf("write permission for %s denied\n", filename);
            iput(mountedINode);
            return -3;
        }
    }
    
    // check if already open -- using mounted value in MINODE
    if (mountedINode->mounted < 0) { // not in use
        printf("%s is not already open\n", filename);
    } else if((mountedINode->mounted == READ_MODE) && (mode == READ_MODE)) { // open for reading only
        printf("%s is already open for reading\n", filename);
    } else {
        printf("%s is currently open for writing\n", filename);
        iput(mountedINode);
        return -4;
    }

    // allocate a free OpenFileTable
    int i;
    OFT * openedFileTable = NULL;
    for (i = 0; i < NFD; i++) {
        if (running->fd[i] == NULL) {
            running->fd[i] = (OFT *) malloc(sizeof(OFT));
            openedFileTable = running->fd[i];
            break;
        }
    }
    if (openedFileTable == NULL) {
        printf("The kernel screams in abject terror at this occurence.\nError: process has too many files open.\n");
        iput(mountedINode);
        return -5;
    }

    // initialize the OpenFileTable
    openedFileTable->mode = mode;
    openedFileTable->mptr = mountedINode;
    openedFileTable->refCount = 1;
    openedFileTable->offset = (mode == 3) ? mountedINode->INODE.i_size : 0;
    if (mode == WRITE_MODE) {
        printf("truncating file: %s\n", filename);
        truncate(mountedINode);
    }

    // tell the MINODE how it was opened
    mountedINode->mounted = mode;

    // update the file's time fields
    mountedINode->INODE.i_atime = time(0L);
    if (mode != READ_MODE) {
        mountedINode->INODE.i_mtine = mountedINode->INODE.i_atime;
    }

    // mark MINODE as dirty
    mountedINode->dirty = 1;

    printf("%s opened for ", filename);

    if (mode == READ_MODE) {
        printf("read\n");
    } else if (mode == WRITE_MODE) {
        printf("write\n");
    } else if (mode == READ_WRITE_MODE) {
        printf("read+write\n");
    } else {
        printf("append\n");
    }

    return i;
}

int truncate(MINODE * mountedINode) {
    printf("freeing data blocks of inode %d\n", mounted->ino);
    INODE * pInode = &(mounted->INODE);
    // for now we assume no indirect blocks
    int numBlocks = pInode->i_blocks;
    for (int i = 0; i < numBlocks; i++) {
        bdalloc(mounted->dev, pInode->i_block[i]);
    }
    idalloc(mounted->dev, mounted->ino);

}

int close_file()

