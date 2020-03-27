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
    printf("Inode: %d Bitmap: %d\n", allocatedInode, allocatedBlock);

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
    /*
    __u32 inode
    __u16 rec_len
    __u8 name_len
    __u8 file_type
    char name []
    */
    //u32 next = sizeof(this)/sizeof(char);
    typedef struct bufferValues __attribute__((__packed__)){
        u32 childInode;
        u16 childRecLen;
        u8 childNameLen;
        char childName[2];
        u32 parInode;
        u16 parRecLen;
        u8 parNameLen;
        char parName[3];
    } BufferValues;

    BufferValues vals = {
        allocatedInode, 12, 2, ".",
        parentInode->ino, (BLKSIZE - 12), 3, ".."
    };

    char buffer[BLKSIZE];
    memcpy(buffer, (const unsigned char *)(&vals), sizeof(vals));

    put_block(parentInode->dev, allocatedBlock, buffer);
    enter_name(parentInode, allocatedInode, childName);
}

int enter_name(MINODE * parentInode, int childInodeNum, char * childName) {
    typedef struct  __attribute__((__packed__)) {
        u32 inode;
        u16 rec_len;
        u8 name_len;
        char name[strlen(childName)];
    } NewEntry;
    struct NewEntry new = {childInodeNum, 0, strlen(childName), childName};

    char buffer[BLKSIZE];
    u32 needed_length = 4*((11+strlen(childName))/4);

    for(int i = 0; i < parentInode->INODE->i_blocks; i++) {
        if (parentInode->INODE->i_block[i] == 0) break;

        get_block(parentInode->dev, parentInode->i_block[i], buffer);
        char * cp = buffer;
        dp = (DIR *) cp;

        printf("Stepping to last entry in data block...\n")
        while(cp + dp->rec_len < buffer + BLKSIZE) {
            cp += dp->rec_len;
            dp = (DIR *) cp;
        }

        u32 new_ideal_length = 4*((11 + dp->name_len)/4);
        u32 remaining_length = dp->rec_len - new_ideal_length;
        if (remaining_length >= needed_length) {
            /* TODO: push new directory information into the buffer, then push to the block. */
            put_block(parentInode->dev, parentInode->INODE->i_block[i], buffer);
            return;
        }
    }

    
}