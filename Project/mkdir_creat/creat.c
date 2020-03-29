int creatFile(MINODE * parentInode, char * childName);

int tryCreat(char * path) {
    MINODE * start = NULL;
    if (path[0] == '/') {
        start = root;
        dev = root->dev;
    } else {
        start = running->cwd;
        dev = running->cwd->dev;
    }

    char * path2 = strdup(path);

    char * childName = basename(path);
    char * parentPath = dirname(path2);

    int parentInodeNum = getino(parentPath);
    MINODE * parentMInode = iget(dev, parentInodeNum);

    if (S_ISDIR(parentMInode->INODE.i_mode)) {
        if (search(parentMInode, childName) == 0) {
            creatFile(parentMInode, childName);
            iput(parentMInode);
            free(path2);
            return 1;
        } else {
            printf("%s already exists in %s\n", childName, parentPath);
            free(path2);
            return 1;
        }
    } else {
        printf("%s is not a directory\n", parentPath);
        free(path2);
        return 0;
    }
}

int creatFile(MINODE * parentInode, char * childName) {
    
}