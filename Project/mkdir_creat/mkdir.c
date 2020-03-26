int makedir(char * path) {
    MINODE * start = NULL;
    if (path[1] == ".") {
        start = running->cwd;
        dev = running->cwd->dev;
    } else {
        start = root;
        dev = root->dev;
    }

    char * path2 = strdup(path);

    char * childName = basename(path);
    char * parentPath = dirname(path2);

    int parentInodeNum = getino(parentPath);
    MINODE * parentInode = iget(parentPath);

    /* TODO: Verify parent is directory,
     * and child doesn't exist in the directory. */

    makeDirectory(parentInode, childName);
    iput(parentInode);
    free(path2);
}

int makeDirectory(MINODE * parentInode, char * childName) {
    MINODE * mounted;
    int allocatedInode = ialloc(dev);
    int allocatedBlocks = balloc(dev);
    printf("Inode: %d Bitmap: %d\n", allocatedInode, allocatedBitmap);

    mounted = iget(dev, allocatedInode);
    pInode = mounted->INODE;

    pInode->i_mode = 040755;
    pInode->i_uid  = running->uid;
    pInode->i_gid  = running->gid;
    pInode->i_size = BLKSIZE;
    pInode->i_links_count = 2; //for . (this dir), and .. (parent dir)
    pInode->i_atime = time(0L);
    pInode->i_ctime = pInode->i_atime;
    pInode->i_mtime = pInode->i_atime;

    pInode->i_blocks = 2;
    pInode->i_block[0] = allocatedBlocks;             // new DIR has one data block   
    for (int i = 1; i < 14; i++) {
        pInode->i_block[i] = 0; 
    }

    mounted->dirty = 1;               // mark minode dirty
    iput(mounted);                    // write INODE to disk
    
}