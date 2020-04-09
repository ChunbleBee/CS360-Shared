int tryRemoveDirectory(char * path);
int removeDirectory(MINODE * parentInode, MINODE * childInode, char * childName);
int removeChild(MINODE * parentInode, char * childName);

int tryRemoveDirectory(char * path) { // rmdir
    int outcome = -1;
    if (path[0] = '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }

    char * pathBack = strdup(path);
    char * parentPath = dirname(parentPath);
    char * childName = basename(path);
    if (strcmp(parentPath, "") == 0) parentPath = ".";

    MINODE * parentMInode = iget(dev, getino(parentPath));

    if ( parentMInode != NULL && S_ISDIR(parentMInode->INODE.i_mode) ) {
        int childInodeNum = search(parentMInode, childName);

        if (childInodeNum != 0) {
            MINODE * childMInode = iget(dev, childInodeNum);
            
            if (childMInode != NULL) {
                if (S_ISDIR(childMInode->INODE.i_mode)) {
                    if (running->uid == 0 ||
                        (running->uid == childMInode->INODE.i_uid &&
                        running->uid == parentMInode->INODE.i_uid)
                    ) {
                        outcome = removeDirectory(parentMInode, childMInode, childName);
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
        printf("Error: no such directory path '%s/%s' found\n", parentPath, childName);
    } else if (outcome == -2) {
        printf("Error: cannot delete another user's directory: '%s/%s'\n", parentPath, childName);
    } else if (outcome == -3) {
        printf("Error: failed to remove directory '%s/%s'\n", parentPath, childName);
    }

    free(pathBack);
    return outcome;
}

int removeDirectory(MINODE * parentMInode, MINODE * childMInode, char * childName) {
    int outcome = -3;
    printf("inside removeDirectory()\nrefCount: %d, iLinks: %d\n", childMInode->refCount, childMInode->INODE.i_links_count);
    getchar();
    if (
        childMInode->refCount == 1 &&
        childMInode->INODE.i_links_count <= 2
    ) {
        //refCount == 1 means only this process is using it.
        //links_count <= 2 means only has self link, and link from parent.
        printf("Inside if\n");
        getchar();
        int numEntries = 0;
        char buffer[BLKSIZE];

        printf("at for\n");
        for (int i = 0; i < 15; i++) {
            if (childMInode->INODE.i_block[i] == 0 || numEntries > 2) {
                break;
            }

            get_block(childMInode->dev, childMInode->INODE.i_block[i], buffer);
            char * cp = buffer;
            dp = (DIR *) cp;
            while (cp + dp->rec_len < buffer + BLKSIZE) {
                numEntries++;
                cp += dp->rec_len;
                dp = (DIR *) cp;
            }
        }

        printf("attempting to free inodes and blocks\n");
        getchar();
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
    char buffer[BLKSIZE];
    char * curBytePtr = NULL;
    DIR * curDirEnt = NULL;
    DIR * prevDirEnt = NULL;

    for (int i = 0; i < 15; i++) {
        // Shouldn't happen, but useful printout should it occur anyways.
        if (parentMInode->INODE.i_block[i] == 0) {
            printf("Couldn't find the name in any allocated data block =/ INODE: %d NAME: %s\n", parentMInode->ino, childName);
            break;
        }
printf("is this the infinite loop?\n");
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
            // we found the corect directory entry node and exited the while loop early.
            if (
                // (prevDirEnt != NULL) || // this just means its first. not only
                (curDirEnt->rec_len != BLKSIZE)
            ) {
                int removedRecordLength = curDirEnt->rec_len;
                //prevDirEnt = curDirEnt;
                //curBytePtr += removedRecordLength;
                //curDirEnt = (DIR *) curBytePtr;

                while(curBytePtr + removedRecordLength - buffer < BLKSIZE) {

                    curBytePtr = ((char *) prevDirEnt) + prevDirEnt->rec_len;
                    prevDirEnt = (DIR *) curBytePtr;
                    curBytePtr = ((char *) curDirEnt) + curDirEnt->rec_len;
                    curDirEnt = (DIR *) curBytePtr;
                    
                    memcpy(prevDirEnt, curDirEnt, curDirEnt->rec_len);
                }
                prevDirEnt->rec_len += removedRecordLength;
            } else {
                int deallocatedBlock = parentMInode->INODE.i_block[i];
                for(; i < 11; i++) {
                    parentMInode->INODE.i_block[i] = parentMInode->INODE.i_block[i+1];
                }
                parentMInode->INODE.i_block[i+1] = 0;
                bdalloc(parentMInode->dev, deallocatedBlock);
            }

            put_block(parentMInode->dev, parentMInode->INODE.i_block[i], buffer);
            outcome = 1;
            break;
        }
    }

    if (outcome == -1) printf("Something failed in removeChild D=\n");
    return outcome;
}