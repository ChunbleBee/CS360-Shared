/*********** util.c file ****************/

int truncate(MINODE * mountedINode);

int get_block(int dev, int blk, u8 *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}
int put_block(int dev, int blk, u8 *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }

  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

// return minode pointer to loaded INODE
MINODE * iget(int device, int ino) {
    int i;
    MINODE *mip;
    char buf[BLKSIZE];
    int blk, offset;
    INODE *ip;

    for (i=0; i < NMINODE; i++){
        mip = &minode[i];
        if (mip->dev == device && mip->ino == ino){
            mip->refCount++;
            //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
            return mip;
        }
    }

    for (i=0; i < NMINODE; i++){
        mip = &minode[i];
        if (mip->refCount == 0){
            //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
            mip->refCount = 1;
            mip->dev = device;
            mip->ino = ino;

            // get INODE of ino into buf[ ]    
            blk    = (ino-1)/8 + inode_start;
            offset = (ino-1) % 8;

            //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

            get_block(device, blk, buf);
            ip = (INODE *)buf + offset;
            // copy INODE to mp->INODE
            mip->INODE = *ip;
            return mip;
        }
    }

    printf("PANIC: no more free minodes\n");
    return 0;
}

void iput(MINODE *mip) {
    int i, block, offset;
    char buffer[BLKSIZE];
    INODE *ip;

    mip->refCount--;

    if (mip->refCount > 0)  // minode is still in use
        return;
    if (!mip->dirty)        // INODE has not changed; no need to write back
        return;

    /* write INODE back to disk */
    /***** NOTE *******************************************
    For mountroot, we never MODIFY any loaded INODE
                    so no need to write it back
    FOR LATER WROK: MUST write INODE back to disk if refCount==0 && DIRTY

    Write YOUR code here to write INODE back to disk
    ********************************************************/
    block  = (mip->ino - 1) / 8 + inode_start;
    offset = (mip->ino - 1) % 8;
    get_block(mip->dev, block, buffer);
    ip = (INODE *)buffer + offset;
    *ip = mip->INODE;
    put_block(mip->dev, block, buffer);
} 

int search(MINODE *mip, char *name) {
    char *cp, c, sbuf[BLKSIZE], temp[256];
    DIR *dp;
    INODE *ip;

    printf("search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);
    ip = &(mip->INODE);

    /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

    get_block(mip->dev, ip->i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    printf("  ino   rlen  nlen  name\n");

    while (cp - sbuf < BLKSIZE) {
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        printf("%4d  %4d  %4d    %s  in search\n", 
            dp->inode, dp->rec_len, dp->name_len, temp);
        if (strcmp(temp, name) == 0) {
            printf("found %s : ino = %d\n", temp, dp->inode);
            return dp->inode;
        }

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    return 0;
}

int getino(char *pathname) {
    int i, j, ino, blk, disp;
    char buf[BLKSIZE];
    INODE *ip;
    MINODE *mip;
    MTABLE * p_mtable;

    printf("getino: pathname=%s\n", pathname);
    if (strcmp(pathname, "/") == 0) return 2;

    // starting mip = root OR CWD
    if (pathname[0] == '/') {
        mip = root;
        dev = root->dev;
    } else {
        mip = running->cwd;
        dev = running->cwd->dev;
    }

    mip->refCount++;         // because we iput(mip) later

    tokenize(pathname);

    for (i=0; i<n; i++){
        printf("===========================================\n");
        printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

        if ((strcmp(name[i], "..") == 0) &&
            (mip->dev != root->dev) &&
            (mip->ino == 2)
        ) {
            printf("UP cross mounting point\n");
            for (j = 0; j < NMTABLE; j++) {
                p_mtable = &mtable[j];
                if (p_mtable->dev == mip->dev) {
                    printf("dev %d mounted at mtable[%d]\n", mip->dev, j);
                    break;
                }
            }
            if (j == NMTABLE) {
                printf("ERROR IN getino()!!! - mtable not found\n");
                exit(2);
            }
            iput(mip);
            dev = p_mtable->mptr->dev;
            ino = search(p_mtable->mptr, "..");
            mip = iget(dev, ino);
            continue;
        }

        ino = search(mip, name[i]);

        if (ino==0){
            iput(mip);
            printf("name %s does not exist\n", name[i]);
            return 0;
        }
        iput(mip);                // release current mip
        mip = iget(dev, ino);     // get next mip

        if (mip->mounted == 1) {
            printf("DOWN cross mounting point\n");
            p_mtable = mip->mptr;
            iput(mip);
            dev = p_mtable->dev;
            ino = 2;
            mip = iget(dev, ino);
        }
    }

    iput(mip);                   // release mip  
    return ino;
}

/***************** WE WROTE THIS ****************/
int findmyname(MINODE *parent, u32 myino, char *myname) {
    char buffer[BLKSIZE], * current = buffer;
    DIR * dirPtr = (DIR *) current;

    get_block(parent->dev, parent->INODE.i_block[0], buffer);

    while(myino != dirPtr->inode) {
        current += dirPtr->rec_len;
        dirPtr = (DIR *) current;
    }
    strncpy(myname, dirPtr->name, dirPtr->name_len);
    myname[dirPtr->name_len] = '\0';
}
/***********************************************/

/************** WE ADDED BITMAP FUNCTIONS ******************/

int tst_bit(u8 *buf, int bit) {
    int bytenumber = bit / 8;
    int bitnumber  = bit % 8;
    if (buf[bytenumber] & (1 << bitnumber))
        return 1;
    else
        return 0;   
}

int set_bit(u8 *buf, int bit) {
    int bytenumber = bit / 8;
    int bitnumber  = bit % 8;
    buf[bytenumber] |= (1 << bitnumber);
}

int clr_bit(u8 *buf, int bit) {
    int bytenumber = bit / 8;
    int bitnumber  = bit % 8;
    buf[bytenumber] &= ~(1 << bitnumber);
}

/************** ALLOCATION FUNCTIONS **********************/
// allocate an inode number from inode_bitmap
int ialloc(int device)  {
    int  i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(device, imap, buf);

    for (i=0; i < ninodes; i++) {
        if (tst_bit(buf, i)==0) {
            set_bit(buf, i);
            put_block(device, imap, buf);
            printf("allocated ino = %d\n", i+1); //bits count from 0; ino from 1
            sp->s_free_inodes_count--;
            gp->bg_free_inodes_count--;
            return i+1;
        }
    }
    return 0;
}

int balloc(int device) {
    u32 total_blocks = sp->s_blocks_count;
    u8 buf[BLKSIZE];
    get_block(device, bmap, buf);
    for (int i=0; i < total_blocks; i++) {
        if (tst_bit(buf, i) == 0) {
            set_bit(buf, i);
            put_block(device, bmap, buf);
            sp->s_free_blocks_count--;
            gp->bg_free_blocks_count--;

            get_block(device, i + 1, buf);
            memset(buf, 0, BLKSIZE);
            put_block(device, i + 1, buf);
            return i+1;
        }
    }
    return 0;
}

/********************* DEALLOCATION FUNCTIONS ****************************/
int idalloc(int device, int inodeNum) {
    char buffer[BLKSIZE];

    if (inodeNum > ninodes) {
        printf("Error: inode number #%d out of range\n", inodeNum);
        return 0;
    }

    get_block(device, imap, buffer);
    clr_bit(buffer, inodeNum - 1);
    put_block(device, imap, buffer);

    sp->s_free_inodes_count++;
    gp->bg_free_inodes_count++;

    return 1;
}

int bdalloc(int device, int block) {
    u32 total_blocks = sp->s_blocks_count;
    char buffer[BLKSIZE];
    get_block(device, bmap, buffer);

    // if (block > total_blocks || tst_bit(buffer, block) == 0) {
    //    if (block > total_blocks)
    //       printf("-- e1 -- block out of range.\n");
    //    if (tst_bit(buffer, block) == 0)
    //       printf("-- e2 -- block isn't allocated.\n");
    //    printf("Error: failure in bdalloc(). block number #%d\n", block);
    //    return 0;
    // }
    if (block > total_blocks || tst_bit(buffer, block - 1) == 0) {
        printf("Error: block number #%d out of range\n", block);
        return 0;
    }
    clr_bit(buffer, block - 1);
    put_block(device, bmap, buffer);

    sp->s_free_blocks_count++;
    gp->bg_free_blocks_count++;

    return 1;
}

int freeInodeAndBlocks(MINODE * mounted) {
    printf("freeing inode and data blocks of %d\n", mounted->ino);
    truncate(mounted);
    printf("freeing inode: %d", mounted->ino);
    idalloc(mounted->dev, mounted->ino);
}


/*********** OTHER UTILITY FUNCTIONS *********/
int min(int x, int y) {
    return (x < y) ? x : y;
}


int printBlockList (MINODE * minode) {
    INODE * pInode = &(minode->INODE);
    int iBuffer[256], dBuffer[256];
    int numBlocks = pInode->i_blocks / 2;
    int i = 0, d = 0; // d is the number of indirect blocks accessed
    printf("[INODE %4d] and has %d (1024 byte) blocks\n",
        minode->ino, pInode->i_blocks / 2);
    printf("  direct blocks :");
    for (i = 0; i < 12; i++) {
        if (i == numBlocks) break;
        if (i % 6 == 0) printf("\n    ");
        printf("%4d  ", pInode->i_block[i]);
    }    
    if (i < numBlocks) { // i == 12, there are indirect blocks
        printf("\ni_block[12] = %d : indirect blocks :", pInode->i_block[12]);
        get_block(minode->dev, pInode->i_block[12], (u8 *) iBuffer);
        i++; d++;
        for (; i < d + 256 + 12; i++) {
            if (i == numBlocks) break;
            if ((i - (1 + 12)) % 10 == 0) printf("\n    ");
            printf("%4d  ", iBuffer[i - (1 + 12)]);
        }
    }
    if (i < numBlocks) { // i == 256 + 1 + 12, there are double indirect blocks
        printf("\ni_block[13] = %d", pInode->i_block[13]);
        get_block(minode->dev, pInode->i_block[13], (u8 *) dBuffer);
        i++; d++;
        int iBlock;
        int ix = 0; // ix is the index of the indirect block in the double
                    // indirect buffer
        int bx = 0; // bx is the index of the data block on the indirect buffer
        for (; i < d + 256*256 + 256 + 12; i++) {
            if (i == numBlocks) break;
            if (bx % 256 == 0) { // (i - d - 256 - 12) % 256 == 0) {
                bx = 0;
                // ix = (i - d - 256 - 12) / 256;
                iBlock = dBuffer[ix];
                printf("\n    i_block[13][%u] = %u : double indirect blocks :",
                    ix, iBlock);
                get_block(minode->dev, iBlock, (u8 *) iBuffer);
                i++; d++; ix++; // set ix to next ix
                if (i == numBlocks) break;
            }
            if (bx % 10 == 0) printf("\n        ");
            printf("%4u  ", iBuffer[bx]);
            bx++;
        }
    }
    printf("\n");
    return numBlocks;
}
