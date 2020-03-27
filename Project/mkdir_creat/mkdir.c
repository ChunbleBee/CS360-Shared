int tryMakeDirectory(char * path) {
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
            makeDirectory(parentMInode, childName);
            iput(parentMInode);
            free(path2);
            return 1;
        } else {
            printf("%s already exists in %s\n", childName, parentPath);
            free(path2);
            return 0;
        }
    } else {
        printf("%s is not a directory\n", parentPath);
        free(path2);
        return 0;
    }
}

int makeDirectory(MINODE * parentInode, char * childName) {
    MINODE * mounted;
    int allocatedInode = ialloc(dev);
    int allocatedBlock = balloc(dev);
    printf("Inode: %d Bitmap: %d\n", allocatedInode, allocatedBlock;

    mounted = iget(dev, allocatedInode);
    INODE * pInode = &(mounted->INODE);

    pInode->i_mode = 040755;
    pInode->i_uid  = running->uid;
    pInode->i_gid  = running->gid;
    pInode->i_size = BLKSIZE;
    pInode->i_links_count = 2; //for . (this dir), and .. (parent dir)
    pInode->i_atime = time(0L);
    pInode->i_ctime = pInode->i_atime;
    pInode->i_mtime = pInode->i_atime;

    pInode->i_blocks = 2;
    pInode->i_block[0] = allocatedBlock;             // new DIR has one data block   
    for (int i = 1; i < 14; i++) {
        pInode->i_block[i] = 0; 
    }

    mounted->dirty = 1;               // mark minode dirty
    iput(mounted);                    // write INODE to disk
    DIR this;
}