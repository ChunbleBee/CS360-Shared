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
            int created = tryCreate(newPath);
            if (created == 0) {
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

//int readlink();
