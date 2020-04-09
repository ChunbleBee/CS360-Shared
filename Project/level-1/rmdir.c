int tryRemoveDirectory(char * path);
int removeDirectory(MINODE * parentInode, MINODE * childInode);
int removeChild(MINODE * parentInode, int childInode, char * childName);

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
                    // check for user permissions.
                    outcome = removeDirectory(parentMInode, childMInode);
                    parentMInode->refCount--;
                    parentMInode->dirty = 1;
                    iput(parentMInode);
                    return outcome;
                } else {
                    iput(childInode);
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

int removeDirectory(MINODE * parentInode, MINODE * childInode) {
    int outcome = 0;

    

    return outcome;
}

int removeChild(MINODE * parentInode, MINODE * childInode) {
    int outcome = 0;
    return outcome;
}