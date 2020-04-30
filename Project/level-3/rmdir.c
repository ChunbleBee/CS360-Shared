int tryRemoveDirectory(char * path);
int removeDirectory(MINODE * parentInode,
    MINODE * childInode, char * childName);
int removeChild(MINODE * parentInode, char * childName);

int tryRemoveDirectory(char * path) { // rmdir
    printf("This is the initial path: %s\n", path);
    int outcome = -1;
    if (path[0] == '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }

    char * path2 = strdup(path);

    char * childName = basename(path);
    char * parentPath = dirname(path2);
    if (strcmp(parentPath, "") == 0) parentPath = ".";

    printf("Parent Path: %s, Child path: %s\n", parentPath, childName);

    MINODE * parentMInode = iget(dev, getino(parentPath));

    if ( parentMInode != NULL && S_ISDIR(parentMInode->INODE.i_mode) ) {
        int childInodeNum = search(parentMInode, childName);

        if (childInodeNum != 0) {
            MINODE * childMInode = iget(dev, childInodeNum);
            
            if (childMInode != NULL) {
                if (S_ISDIR(childMInode->INODE.i_mode)) {
                    if ((running->pid == 0) ||
                        (0)
                    ) {
                        outcome = removeDirectory(parentMInode,
                            childMInode, childName);
                    } else {
                        printf("incorrect permissions\n");
                        outcome = -2;
                    }
                } else {
                    printf("child isn't directory\n");
                }
                iput(childMInode);
            }
        } else {
            printf("child doesn't exist\n");
        }

        iput(parentMInode);
    } else {
        printf("parent not directory\n");
    }


    if (outcome == -1) {
        printf("Error: no such directory path '%s/%s' found\n",
            parentPath, childName);
    } else if (outcome == -2) {
        printf("Error: cannot delete another user's directory: '%s/%s'\n",
            parentPath, childName);
    } else if (outcome == -3) {
        printf("Error: failed to remove directory '%s/%s'\n",
            parentPath, childName);
    }

    free(path2);
    return outcome;
}

int removeDirectory(MINODE * parentMInode,
    MINODE * childMInode, char * childName) {
    int outcome = -3;
    printf("inside removeDirectory()\nrefCount: %d, iLinks: %d\n",
        childMInode->refCount, childMInode->INODE.i_links_count);
    // getchar();
    if (
        childMInode->refCount == 1 &&
        childMInode->INODE.i_links_count <= 2
    ) {
        //refCount == 1 means only this process is using it.
        //links_count <= 2 means only has self link, and link from parent.
        printf("Inside if\n");
        // getchar();
        int numEntries = 0;
        u8 buffer[BLKSIZE];
        printf("at for\n");
        for (int i = 0; i < 15; i++) {
            if (childMInode->INODE.i_block[i] == 0 || numEntries > 2) {
                break;
            }

            get_block(childMInode->dev, childMInode->INODE.i_block[i], buffer);
            u8 * cp = buffer;
            dp = (DIR *) cp;
            while (cp + dp->rec_len < buffer + BLKSIZE) {
                numEntries++;
                cp += dp->rec_len;
                dp = (DIR *) cp;
            }
        }

        printf("attempting to free inodes and blocks\n");
        // getchar();
        childMInode->dirty = 1;
        freeInodeAndBlocks(childMInode);
        outcome = (removeChild(parentMInode, childName) == -1) ? outcome : 0;
        parentMInode->dirty = 1;
    }
    printf("freeing inode\n");
    return outcome;
}

int removeChild(MINODE * parentMInode, char * childName) {
    int outcome = -1;
    u8 buffer[BLKSIZE];
    u8 * curBytePtr = NULL;
    u8 * prevBytePtr = NULL;
    DIR * curDirEnt = NULL;
    DIR * prevDirEnt = NULL;

    for (int i = 0; i < 15; i++) {
        // Shouldn't happen, but useful printout should it occur anyways.
        if (parentMInode->INODE.i_block[i] == 0) {
            printf("Couldn't find the name in any allocated data block");
            printf(" =/ INODE: %d NAME: %s\n", parentMInode->ino, childName);
            break;
        }
        get_block(parentMInode->dev, parentMInode->INODE.i_block[i], buffer);

        curBytePtr = buffer;
        curDirEnt = (DIR *) curBytePtr;
        prevDirEnt = NULL;

        while (
            (curBytePtr - buffer < BLKSIZE) &&
            (strncmp(childName, curDirEnt->name, curDirEnt->name_len) != 0)
        ) {
            prevDirEnt = curDirEnt;
            curBytePtr += curDirEnt->rec_len;
            curDirEnt = (DIR *) curBytePtr;
        }

        if (curBytePtr - buffer < BLKSIZE) {
            // we found the corect directory entry node
            // and exited the while loop early.
            if (curDirEnt->rec_len != BLKSIZE) {
                prevBytePtr = (u8 *)prevDirEnt;
                u16 removedRecordLength = curDirEnt->rec_len;

                if (curBytePtr + removedRecordLength < buffer + BLKSIZE) {
                    //first or mid entry.
                    prevBytePtr = curBytePtr;
                    curBytePtr += removedRecordLength;
                    prevDirEnt = (DIR *) prevBytePtr;
                    curDirEnt = (DIR *) curBytePtr;

                    while(curBytePtr + removedRecordLength <= buffer+BLKSIZE) {
                        u16 recordLength = curDirEnt->rec_len;
                        // printf("byte location: %d, record length: %u\n",
                        //     prevBytePtr + removedRecordLength, recordLength);
                        // printf("prevDirEnt info: %.*s, inode: %u, reclen: %u,
                        //     byteptr: %d\n", prevDirEnt->name_len,
                        //     prevDirEnt->name, prevDirEnt->inode,
                        //     prevDirEnt->rec_len, prevBytePtr);
                        // printf("curDirEnt info: %.*s, inode: %u, reclen: %u,
                        //     byteptr: %d\n", curDirEnt->name_len,
                        //     curDirEnt->name, curDirEnt->inode,
                        //     curDirEnt->rec_len, curBytePtr);
                        // getchar();
                        memcpy(prevBytePtr, curBytePtr, recordLength);
                        curBytePtr += recordLength;
                        curDirEnt = (DIR *) curBytePtr;

                        if (curBytePtr + removedRecordLength < buffer+BLKSIZE) {
                            prevBytePtr += recordLength;
                            prevDirEnt = (DIR *) prevBytePtr;
                        }
                    }
                }

                prevDirEnt->rec_len += removedRecordLength;
            } else {
                int deallocatedBlock = parentMInode->INODE.i_block[i];
                for(; i < 11; i++) {
                    parentMInode->INODE.i_block[i] =
                        parentMInode->INODE.i_block[i+1];
                }
                parentMInode->INODE.i_block[i+1] = 0;
                bdalloc(parentMInode->dev, deallocatedBlock);
            }

            put_block(parentMInode->dev,parentMInode->INODE.i_block[i],buffer);
            outcome = 1;
            break;
        }
    }

    if (outcome == -1) printf("Something failed in removeChild D=\n");
    return outcome;
}
