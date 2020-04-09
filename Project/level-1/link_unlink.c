int tryLink(char * oldPath, char * newPath) { // link
    if (oldPath[0] == '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }

    char oldPathCopy[128];
    strcpy(oldPathCopy, oldPath);

    char * oldChildName = basename(oldPath);
    char * oldParentPath = dirname(oldPathCopy);

    int oldParentInodeNum = getino(oldParentPath);
    MINODE * oldParentMInode = iget(dev, oldParentInodeNum);

    if (S_ISDIR(oldParentMInode->INODE.i_mode)) {
        int oldChildInodeNum = search(oldParentMInode, oldChildName);
        if (oldChildInodeNum != 0) {
            MINODE * oldChildMInode = iget(dev, oldChildInodeNum);
            if (S_ISDIR(oldChildMInode->INODE.i_mode) == 0) {
                // try new path here //
                int newDev;
                if (newPath[0] == '/') {
                    newDev = root->dev;
                } else {
                    newDev = running->cwd->dev;
                }
                if (newDev == dev) {
                    char newPathCopy[128];
                    strcpy(newPathCopy, newPath);

                    char * newChildName = basename(newPath);
                    char * newParentPath = dirname(newPathCopy);

                    int newParentInodeNum = getino(newParentPath);
                    MINODE * newParentMInode = iget(dev, newParentInodeNum);

                    if (S_ISDIR(newParentMInode->INODE.i_mode)) {
                        if (search(newParentMInode, newChildName) == 0) {
                            enter_name(newParentMInode, oldChildInodeNum, newChildName);
                            oldChildMInode->INODE.i_links_count++;
                            oldChildMInode->dirty = 1;
                            iput(newParentMInode);
                            iput(oldChildMInode);
                            iput(oldParentMInode);
                            return 1;
                        } else {
                            printf("%s already exists in %s\n", newChildName, newParentPath);
                            iput(newParentMInode);
                            iput(oldChildMInode);
                            iput(oldParentMInode);
                            return 0;
                        }
                    } else {
                        printf("%s is not a directory\n", newParentPath);
                        iput(newParentMInode);
                        iput(oldChildMInode);
                        iput(oldParentMInode);
                        return 0;
                    }
                } else {
                    printf("%s and %s are not on the same device\n", oldChildName, newPath);
                    iput(oldChildMInode);
                    iput(oldParentMInode);
                    return 0;
                }
                ///////////////////////
            } else {
                printf("%s is a directory\n", oldChildName);
                iput(oldChildMInode);
                iput(oldParentMInode);
                return 0;
            }
        } else {
            printf("%s does not exist in %s\n", oldChildName, oldParentPath);
            iput(oldParentMInode);
            return 0;
        }
    } else {
        printf("%s is not a directory\n", oldParentPath);
        iput(oldParentMInode);
        return 0;
    }
}