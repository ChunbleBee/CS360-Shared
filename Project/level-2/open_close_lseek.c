int open_file(char * filename, int mode) {
    if (mode < 0 || mode > 3) {
        printf("Error: open mode %d is not defined\n", mode);
        return -10;
    }
    if (filename[0] == '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }
    int inodeNum = getino(filename);
    if (inodeNum == 0) {
        if (mode == READ_MODE) {
            printf("Cannot open %s for reading\n", filename);
            return -14;
        }
        char filenameCopy[128];
        strcpy(filenameCopy, filename);
        tryCreate(filenameCopy);
        inodeNum = getino(filename);
    }
    MINODE * mountedINode = iget(dev, inodeNum);

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
    if ((mode == WRITE_MODE) || (mode == READ_WRITE_MODE) ||
        (mode == APPEND_MODE)) {
        if ( (running->uid == 0) || // super user
            ((mountedINode->INODE.i_mode & 0200) &&
                (running->uid == mountedINode->INODE.i_uid)) || // user write
            ((mountedINode->INODE.i_mode & 0020) &&
                (running->uid == mountedINode->INODE.i_gid)) || // group write
             (mountedINode->INODE.i_mode & 0002) // other write
        ) {
            printf("write permission for %s granted\n", filename);
        } else {
            printf("write permission for %s denied\n", filename);
            iput(mountedINode);
            return -3;
        }
    }
    
    // check if already open -- using mounted value in MINODE
    int multiread = 0;
    if (mountedINode->mounted < 0) { // not in use
        printf("%s is not already open\n", filename);
    } else if((mountedINode->mounted == READ_MODE) &&
        (mode == READ_MODE)) { // open for reading only
        multiread = 1;
        printf("%s is already open for reading\n", filename);
    } else {
        printf("%s is currently open for writing\n", filename);
        iput(mountedINode);
        return -4;
    }

    // find previously opened for reading oft
    OFT * prevOpenedOFT;
    if (multiread == 1) {
        for (int i = 0; i < NOFT; i++) { // find the previously opened oft
            prevOpenedOFT = &oft[i];
            if (prevOpenedOFT->mptr != NULL &&
                prevOpenedOFT->mptr->ino == mountedINode->ino
            ) {
                break;
            }
        }
    }

    // allocate a free OpenFileTable
    OFT * openedFileTable = NULL;
    for (int i = 0; i < NOFT; i++) {
        openedFileTable = &oft[i];
        if (openedFileTable->refCount == 0) {
            break;
        }
    }
    if (openedFileTable == NULL) {
        printf("The kernel screams in abject terror at this occurence.\n");
        printf("Error: process has too many files open.\n");
        iput(mountedINode);
        return -5;
    }

    // find an avalible fd in the running proc
    int fd;
    for (fd = 0; fd < NFD; fd++) {
        if (running->fd[fd] == NULL) {
            running->fd[fd] = openedFileTable;
            break;
        }
    }

    // initialize the OpenFileTable
    openedFileTable->mode = mode;
    openedFileTable->mptr = mountedINode;
    if (multiread == 1) {
        // what if it's open multiple times
        prevOpenedOFT->refCount++;
        openedFileTable->refCount = prevOpenedOFT->refCount;
    } else {
        openedFileTable->refCount = 1;
    }
    if (mode == APPEND_MODE) {
        openedFileTable->offset = mountedINode->INODE.i_size;
    } else {
        openedFileTable->offset = 0;
    }
    printf("opened file table offset: %d\n", openedFileTable->offset);
    if (mode == WRITE_MODE) {
        printf("truncating file: %s\n", filename);
        truncate(mountedINode);
    }

    // tell the MINODE how it was opened
    mountedINode->mounted = mode;

    // update the file's time fields
    mountedINode->INODE.i_atime = time(0L);
    if (mode != READ_MODE) {
        mountedINode->INODE.i_mtime = mountedINode->INODE.i_atime;
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

    return fd;
}

int truncate(MINODE * minode) {
    INODE * pInode = &(minode->INODE);
    int iBuffer[256], dBuffer[256];
    int numBlocks = pInode->i_blocks / 2;
    int i = 0, d = 0; // d is the number of indirect blocks accessed
    printf("[INODE %4d] and has %d (1024 byte) blocks\n",
        minode->ino, pInode->i_blocks / 2);
    for (i = 0; i < 12; i++) {
        printf("  freeing direct data blocks:");
        if (i == numBlocks) break;
        if (i % 6 == 0) printf("\n    ");
        printf("%4d  ", pInode->i_block[i]);
        bdalloc(minode->dev, pInode->i_block[i]);
    }
    if (i < numBlocks) { // i == 12, there are indirect blocks
        printf("\ni_block[12] = %d :", pInode->i_block[12]);
        get_block(minode->dev, pInode->i_block[12], (u8 *) iBuffer);
        printf("\nfreeing i_block[12] = %4d\n", pInode->i_block[12]);
        bdalloc(minode->dev, pInode->i_block[12]);
        printf("freeing indirect data blocks :");
        i++; d++;
        for (; i < d + 256 + 12; i++) {
            if (i == numBlocks) break;
            if ((i - (1 + 12)) % 10 == 0) printf("\n    ");
            printf("%4d  ", iBuffer[i - (1 + 12)]);
            bdalloc(minode->dev, iBuffer[i - (1 + 12)]);
        }
    }
    if (i < numBlocks) { // i == 256 + 1 + 12, there are double indirect blocks
        printf("\ni_block[13] = %d", pInode->i_block[13]);
        get_block(minode->dev, pInode->i_block[13], (u8 *) dBuffer);
        printf("freeing i_block[13] = %u\n", pInode->i_block[13]);
        bdalloc(minode->dev, pInode->i_block[13]);
        i++; d++;
        int iBlock;
        int ix = 0; // ix is the index of the indirect block in the double
                    // indirect buffer
        int bx = 0; // bx is the index of the data block on the indirect buffer
        for (; i < d + 256*256 + 256 + 12; i++) {
            if (i == numBlocks) break;
            if (bx % 256 == 0) { // (i - d - 256 - 12) % 256 == 0) {
                bx = 0;
                // ix = (i - d - 256 - 12) / 256;
                iBlock = dBuffer[ix];
                printf("\n    i_block[13][%u] = %u : ",ix, iBlock);
                get_block(minode->dev, iBlock, (u8 *) iBuffer);
                printf("freeing i_block[13][%u] = %u\n", ix, iBlock);
                bdalloc(minode->dev, iBlock);
                i++; d++; ix++; // set ix to next ix
                if (i == numBlocks) break;
                printf("    freeing double indirect data blocks :");
            }
            if (bx % 10 == 0) printf("\n        ");
            printf("%4u  ", iBuffer[bx]);
            bdalloc(minode->dev, iBuffer[bx]);
            bx++;
        }
    }
    printf("\n");
    minode->INODE.i_atime = time(0L);
    minode->INODE.i_mtime = minode->INODE.i_atime;
    minode->INODE.i_size = 0;
    minode->dirty = 1;
    return 1;
}

int close_file(int fileDescriptor) {
    if (fileDescriptor < 0 || fileDescriptor >= NFD) {
        printf("file descriptor %d out of range\n", fileDescriptor);
        return -1;
    }
    if (running->fd[fileDescriptor] == NULL) {
        printf("file descriptor %d is not in use\n", fileDescriptor);
        return -2;
    }
    OFT * openedFileTable = running->fd[fileDescriptor];
    running->fd[fileDescriptor] = NULL;
    for (int i = 0; i < NOFT; i++) {
        if (oft[i].mptr != NULL &&
            oft[i].mptr->ino == openedFileTable->mptr->ino
        ) {
            oft[i].refCount--;
        }
    }
    MINODE * closedMInode = openedFileTable->mptr;
    int refCount = openedFileTable->refCount;

    openedFileTable->mode = -1;
    openedFileTable->mptr = NULL;
    openedFileTable->offset = 0;
    openedFileTable->refCount = 0;

    if (refCount == 0) {
        printf("Closing Inode %d\n", closedMInode->ino);
        closedMInode->mounted = -1;
        iput(closedMInode);
    }
    return 1;
}

int lseek_file(int fileDescriptor, int position) {
    if (fileDescriptor < 0 || fileDescriptor >= NFD) {
        printf("file descriptor %d out of range\n", fileDescriptor);
        return -1;
    }
    if (running->fd[fileDescriptor] == NULL) {
        printf("file descriptor %d is not in use\n", fileDescriptor);
        return -2;
    }
    if (position < 0 || position >=
        running->fd[fileDescriptor]->mptr->INODE.i_size
        ) {
        printf("posision %d is out of range\n", position);
        return -3;
    }
    running->fd[fileDescriptor]->offset = position;
    return 1;
}

int pfd() {
    printf("  fd        mode       offset     INODE\n");
    printf(" ----    ----------    ------    -------\n");
    char mode[12];
    for (int i = 0; i < NFD; i++) {
        if (running->fd[i] != NULL) {
            if (running->fd[i]->mode == READ_MODE) {
                strcpy(mode, "   READ   ");
            } else if (running->fd[i]->mode == WRITE_MODE) {
                strcpy(mode, "   WRITE  ");
            } else if (running->fd[i]->mode == READ_WRITE_MODE) {
                strcpy(mode, "READ_WRITE");
            } else if (running->fd[i]->mode == APPEND_MODE) {
                strcpy(mode, "  APPEND  ");
            }
            printf("% 2d     %.10s    % 5d    [%3d,%3d]\n",
                i,
                mode,
                running->fd[i]->offset,
                running->fd[i]->mptr->dev,
                running->fd[i]->mptr->ino
            );
        }
    }
    printf("----------------------------------------\n");
    return 1;
}

int mydup(int fileDescriptor) {
    if (fileDescriptor < 0 || fileDescriptor >= NFD) {
        printf("file descriptor %d out of range\n", fileDescriptor);
        return -1;
    }
    if (running->fd[fileDescriptor] == NULL) {
        printf("file descriptor %d is not in use\n", fileDescriptor);
        return -2;
    }
    OFT * dupFileTable = NULL;
    for (int i = 0; i < NOFT; i++) {
        dupFileTable = &oft[i];
        if (dupFileTable->refCount == 0) {
            break;
        }
    }
    dupFileTable->mode     = running->fd[fileDescriptor]->mode;
    dupFileTable->mptr     = running->fd[fileDescriptor]->mptr;
    dupFileTable->offset   = running->fd[fileDescriptor]->offset;
    dupFileTable->refCount = running->fd[fileDescriptor]->refCount;
    for (int i = 0; i < NFD; i++) {
        if (running->fd[i] == NULL) {
            running->fd[i] = dupFileTable;
            break;
        }
    }
    for (int i = 0; i < NOFT; i++) {
        if (oft[i].mptr != NULL &&
            oft[i].mptr->ino == dupFileTable->mptr->ino
        ) {
            oft[i].refCount++;
        }
    }

}
