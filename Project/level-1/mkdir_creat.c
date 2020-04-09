int makeDirectory(MINODE * parentInode, char * childName);
int enter_name(MINODE * parentInode, int childInodeNum, char * childName);
int createFile(MINODE * parentInode, char * childName);

int tryCreate(char * path) { // creat
    if (path[0] == '/') {
        dev = root->dev;
    } else {
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
        } else {
            printf("%s already exists in %s\n", childName, parentPath);
        }
        iput(parentMInode);
        free(path2);
        return 1;
    } else {
        printf("%s is not a directory\n", parentPath);
        iput(parentMInode);
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

int tryMakeDirectory(char * path) { // mkdir
    if (path[0] == '/') {
        printf("Starting at the root!\n");
        dev = root->dev;
    } else {
        printf("Starting at the local directory!\n");
        
        dev = running->cwd->dev;
    }

    char * path2 = strdup(path);

    char * childName = basename(path);
    char * parentPath = dirname(path2);
    if (strcmp(parentPath, "") == 0) parentPath = ".";

    int parentInodeNum = getino(parentPath);
    MINODE * parentMInode = iget(dev, parentInodeNum);
    if (S_ISDIR(parentMInode->INODE.i_mode)) {
        if (search(parentMInode, childName) == 0) {
            printf("Found directory, and name available.\n");
            makeDirectory(parentMInode, childName);
            parentMInode->refCount++;
            parentMInode->dirty = 1;
        } else {
            printf("%s already exists in %s\n", childName, parentPath);
        }
        iput(parentMInode);
        free(path2);
        return 1;
    } else {
        printf("%s is not a directory\n", parentPath);
        iput(parentMInode);
        free(path2);
        return 0;
    }
}

int makeDirectory(MINODE * parentInode, char * childName) {
    // fix pino->dev all over
    MINODE * mounted;
    int allocatedInode = ialloc(parentInode->dev);
    int allocatedBlock = balloc(parentInode->dev);
    printf("NOT DARK: Inode: %d Block: %d\n", allocatedInode, allocatedBlock);

    mounted = iget(parentInode->dev, allocatedInode);
    INODE * pInode = &(mounted->INODE);

    pInode->i_mode = 040755;
    pInode->i_uid  = running->uid;
    pInode->i_gid  = running->gid;
    pInode->i_size = BLKSIZE;
    pInode->i_links_count = 2;
    pInode->i_atime = time(0L);
    pInode->i_ctime = pInode->i_atime;
    pInode->i_mtime = pInode->i_atime;

    pInode->i_blocks = 2; //(BLKSIZE/512 > 0) ? BLKSIZE/512 : 1;
    pInode->i_block[0] = allocatedBlock;
    for (int i = 1; i < 15; i++) {
        pInode->i_block[i] = 0; 
    }

    mounted->dirty = 1;

    char buffer[BLKSIZE];
    memset(buffer, '\0', BLKSIZE);
    //Child inode information
    char * cp = buffer;
    dp = (DIR *) cp;
    dp->inode = mounted->ino;
    dp->name_len = 1;
    dp->rec_len = 12;
    strncpy(dp->name, ".", 1);

    //Parent inode information
    cp += dp->rec_len;
    dp = (DIR *) cp;
    dp->inode = parentInode->ino;
    dp->name_len = 2;
    dp->rec_len = BLKSIZE - 12;
    strncpy(dp->name, "..", 2);

    put_block(parentInode->dev, allocatedBlock, buffer);
    parentInode->INODE.i_links_count++;
    enter_name(parentInode, allocatedInode, childName);
    iput(mounted);
}

int enter_name(MINODE * parentInode, int childInodeNum, char * childName) {
    char buffer[BLKSIZE];
    memset(buffer, '\0', BLKSIZE);
    u16 needed_length = 4*((11+strlen(childName))/4);

    int i = 0;
    for (i = 0; i < 12; i++) {
        if (parentInode->INODE.i_block[i] == 0) {
            printf("No other entries in data block...\n");
            break;
        }

        get_block(parentInode->dev, parentInode->INODE.i_block[i], buffer);
        char * cp = buffer;
        dp = (DIR *) cp;

        printf("Stepping to last entry in data block...\n");
        while (cp + dp->rec_len < buffer + BLKSIZE) {
            printf("Checking record: %.*s\n", dp->name_len, dp->name);
            cp += dp->rec_len;
            dp = (DIR *) cp;
        }

        printf("Found last entry: %.*s\n", dp->name_len, dp->name);
        u16 new_ideal_length = 4*((11 + dp->name_len)/4);
        u16 remaining_length = dp->rec_len - new_ideal_length;

        if (remaining_length >= needed_length) {
            dp->rec_len = new_ideal_length;
            cp += dp->rec_len;
            dp = (DIR *)cp;
            dp->inode = childInodeNum;
            dp->rec_len = remaining_length;
            dp->name_len = strlen(childName);
            strncpy(dp->name, childName, dp->name_len);
            put_block(parentInode->dev, parentInode->INODE.i_block[i], buffer);
            return 0;
        }
    }
    printf("Allocating new data block...\n");
    //Reach here, no remaining blocks. Increment number of blocks by 1 and allocate a enw data block
    int allocatedBlock = balloc(parentInode->dev);
    parentInode->INODE.i_blocks++;
    parentInode->INODE.i_block[i] = allocatedBlock;
    parentInode->INODE.i_size += BLKSIZE;

    dp = (DIR *) buffer;
    dp->inode = childInodeNum;
    dp->name_len = strlen(childName);
    dp->rec_len = BLKSIZE;
    strncpy(dp->name, childName, dp->name_len);
    put_block(parentInode->dev, allocatedBlock, buffer);
    return 0;
}
