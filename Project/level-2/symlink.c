int symlink(char * oldPath, char * newPath) {
    if (oldPath[0] == '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }

    char oldPathCopy[60], oldPathCopy2[60];
    strcpy(oldPathCopy, oldPath);
    strcpy(oldPathCopy2, oldPath);

    char * oldChildName = basename(oldPath);
    char * oldParentPath = dirname(oldPathCopy);

    int oldParentInodeNum = getino(oldParentPath);
    MINODE * oldParentMInode = iget(dev, oldParentInodeNum);

    if (S_ISDIR(oldParentMInode->INODE.i_mode)) {
        int oldChildInodeNum = search(oldParentMInode, oldChildName);
        if (oldChildInodeNum != 0) {
            MINODE * oldChildMInode = iget(dev, oldChildInodeNum);
            char newPathCopy[128];
            strcpy(newPathCopy, newPath);
            int created = tryCreate(newPathCopy);
            if (created >= 0) {
                printf("created new file: %s", newPath);
                if (newPath[0] == '/') {
                    dev = root->dev;
                } else {
                    dev = running->cwd->dev;
                }
                int newInodeNum = getino(newPath);
                MINODE * newMInode = iget(dev, newInodeNum);
                newMInode->INODE.i_mode = 0120000 | (oldChildMInode->INODE.i_mode & 0777);
                strcpy(((char *) &(newMInode->INODE.i_block[0])), oldPathCopy2);
                newMInode->INODE.i_size = strlen(oldPathCopy2);
                newMInode->dirty = 1;
                iput(newMInode);
                // is new parent inode dirty? No.
                iput(oldChildMInode);
                iput(oldParentMInode);
                return 1;
            } else {
                printf("failed to create new file: %s\n", newPath);
                iput(oldParentMInode);
                return -3;
            }
        } else {
            printf("%s does not exist in %s\n", oldChildName, oldParentPath);
            iput(oldParentMInode);
            return -2;
        }
    } else {
        printf("%s is not a directory\n", oldParentPath);
        iput(oldParentMInode);
        return -1;
    }
}

int readlink(MINODE * linkMInode) {
    if (S_ISLNK(linkMInode->INODE.i_mode)) {
        strcpy(linkedNameBuf, ((char *) &(linkMInode->INODE.i_block[0])));
        return linkMInode->INODE.i_size;
    } else {
        return -1;
    }
}

int readlinkFromPath(char * path) {
    if (path[0] == '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }
    
    int linkInodeNum = getino(path);
    if (linkInodeNum == 0) {
        printf("%s not found\n", path);
        return -1;
    }
    MINODE * linkMInode = iget(dev, linkInodeNum);
    int linkNameLen = readlink(linkMInode);
    if (linkNameLen < 0) {
        printf("%s is not a symlink\n", path);
        iput(linkMInode);
        return -2;
    }
    // "path -> " is omitted in linux shell
    printf("%s -> %.*s\n\n", path, linkNameLen, linkedNameBuf);
    iput(linkMInode);
    return 1;
}
