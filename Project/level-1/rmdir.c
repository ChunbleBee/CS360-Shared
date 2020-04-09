int tryRemoveDirectory(char * path);
int removeDirectory(MINODE * parentInode, MINODE * childInode, char * childName);
int removeChild(MINODE * parentInode, char * childName);

int tryRemoveDirectory(char * path) {
    int outcome = 0;
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
                if (S_ISDIR(childInode->INODE.i_mode)) {
                    if (running->uid == 0 ||
                        (running->uid == childMInode->INODE.i_uid &&
                        running->uid == parentMInode->INODE.i_uid)
                    ) {
                        outcome = removeDirectory(parentMInode, childMInode, char * childName);
                    } else {
                        outcome = -1;
                    }
                } else {
                    iput(childMInode);
                }
            }
        }

        iput(parentMInode);
    }


    if (outcome == 0) {
        printf("Error: no such directory path '%s/%s' found\n", parentPath, childName);
    } else if (outcome == -1) {
        printf("Error: cannot delete another user's directory: '%s/%s'\n", parentPath, childName);
    } else if (outcome == -2) {
        printf("Error: failed to remove directory '%s/%s'\n", parentPath, childName);
    }

    free(pathBack);
    return outcome;
}

int removeDirectory(MINODE * parentMInode, MINODE * childMInode, char * childName) {
    int outcome = -2;

    if (
        childMInode->refCount == 1 &&
        childMInode->INODE.i_links_count <= 2
    ) {
        //refCount == 1 means only this process is using it.
        //links_count <= 2 means only has self link, and link from parent.

        int numEntries = 0;
        char buffer[BLKSIZE];

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

        freeInodeAndBlocks(childMInode);
        outcome = (removeChild(parentMInode, childName) == 0) ? -2 : 1;
    }

    childMInode->dirty = 1;
    iput(childMInode);
    return outcome;
}

int removeChild(MINODE * parentMInode, char * childName) {
    int outcome = 0;
    char buffer[BLKSIZE];
    char * curPtr = NULL;
    DIR * curDir = NULL;
    DIR * prevDir = NULL;
    int i = 0;

    for (; i < 12; i++) {
        if (parentMInode->INODE.i_block[i] == 0) {
            break;
        }

        get_block(parentMInode->dev, parentMInode->INODE.i_block[i], buffer);

        while (
            curPtr - buffer < BLKSIZE &&
            strncmp(curDir->name, childName, curDir->name_len) != 0
        ) {
            curPtr += curDir->rec_len;
            prevDir = curDir;
            curDir = (DIR *) curPtr;
        }
    }

    if ( parentMInode->INODE.i_block[i] != 0 ) {

    }

    return outcome;
}