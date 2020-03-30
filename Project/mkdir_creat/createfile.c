int createFile(MINODE * parentInode, char * childName);

int tryCreate(char * path) {
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
            createFile(parentMInode, childName);
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

int createFile(MINODE * parentInode, char * childName) {
    MINODE * mounted;
    int allocatedInode = ialloc(parentInode->dev);
    printf("SPARKLES ARE NOT DARK: Inode: %d\n", allocatedInode);

    mounted = iget(parentInode->dev, allocatedInode);
    INODE * pInode = &(mounted->INODE);

    pInode->i_mode = 010644;
    pInode->i_uid  = running->uid;
    pInode->i_gid  = running->gid;
    pInode->i_size = 0;
    pInode->i_links_count = 1;
    pInode->i_atime = time(0L);
    pInode->i_ctime = pInode->i_atime;
    pInode->i_mtime = pInode->i_atime;

    pInode->i_blocks = 0;
    for (int i = 0; i < 15; i++) {
        pInode->i_block[i] = 0;
    }

    mounted->dirty = 1;
    enter_name(parentInode, allocatedInode, childName);
    iput(mounted);
}
