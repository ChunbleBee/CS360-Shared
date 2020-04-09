
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

int tryUnlink(char *path) {
    if (path[0] == '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }
    int childInodeNum = getino(path);
    if (childInodeNum == 0) {
        printf("%s does not exist\n", path);
        return 1;
    }
    MINODE * childMInode = iget(dev, childInodeNum);
    if (S_ISREG(childMInode->INODE.i_mode) || S_ISSLNK(childMInode->INODE.i_mode)) {
        char pathCopy[128];
        strcpy(pathCopy, path);
        char * childName = basename(path);
        char * parentPath = dirname(pathCopy);
        int parentInodeNum = getino(parentPath);
        MINODE * parentMInode = iget(dev, parentInodeNum);
        rm_child(parentMInode, childName);
        parentMInode->dirty = 1;
        iput(parentMInode);
        childMInode->INODE.i_links_count--;
        if (childMInode->INODE.i_links_count > 0) {
            childMInode->dirty = 1;
        } else {
            freeInodeAndBlocks(childMInode->INODE);
        }
    } else {
        printf("%s is not a regular file or a symbolic link\n", path);
        iput(childMInode);
        return 0;
    }
}
