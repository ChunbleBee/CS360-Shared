/*********** util.c file ****************/

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}
int put_block(int dev, int blk, char *buf)
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
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino into buf[ ]    
       blk    = (ino-1)/8 + inode_start;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);
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

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp - sbuf < BLKSIZE) {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;

      printf("%4d  %4d  %4d    %s\n", 
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
   int i, ino, blk, disp;
   char buf[BLKSIZE];
   INODE *ip;
   MINODE *mip;

   printf("getino: pathname=%s\n", pathname);
   if (strcmp(pathname, "/") == 0) return 2;
  
   // starting mip = root OR CWD
   if (pathname[0] == '/') {
      mip = root;
   } else {
      mip = running->cwd;
   }

   mip->refCount++;         // because we iput(mip) later
  
   tokenize(pathname);

   for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);                // release current mip
      mip = iget(dev, ino);     // get next mip
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
   //printf("\n%s\n", myname); //TODO-rm
}
/***********************************************/

int findino(MINODE *mip, u32 *myino) {
   // myino = ino of . return ino of ..
   char buf[BLKSIZE], *cp;   
   DIR *dp;

   get_block(mip->dev, mip->INODE.i_block[0], buf);
   cp = buf; 
   dp = (DIR *)buf;
   *myino = dp->inode;
   cp += dp->rec_len;
   dp = (DIR *)cp;
   return dp->inode;
}

/************** WE ADDED BITMAP FUNCTIONS ******************/

int tst_bit(char *buf, int bit) {
   int bytenumber = bit / 8;
   int bitnumber  = bit % 8;
   if (buf[bytenumber] & (1 << bitnumber))
      return 1;
   else
      return 0;   
}

int set_bit(char *buf, int bit) {
   int bytenumber = bit / 8;
   int bitnumber  = bit % 8;
   buf[bytenumber] |= (1 << bitnumber);
}

int clr_bit(char *buf, int bit) {
   int bytenumber = bit / 8;
   int bitnumber  = bit % 8;
   buf[bytenumber] &= ~(1 << bitnumber);
}

/************** ALLOCATION FUNCTIONS **********************/


int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
  int  i;
  char buf[BLKSIZE];

// read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
        put_block(dev, imap, buf);
        printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
        sp->s_free_inodes_count--;
        gp->bg_free_inodes_count--;
        return i+1;
    }
  }
  return 0;
}

int balloc(int dev) {
   u32 total_blocks = sp->s_blocks_count;
   char buf[BLKSIZE];
   get_block(dev, bmap, buf);
   for (int i=0; i < total_blocks; i++) {
      if (tst_bit(buf, i) == 0) {
         set_bit(buf, i);
         put_block(dev, bmap, buf);
         sp->s_free_blocks_count--;
         gp->bg_free_blocks_count--;
         return i+1;
      }
   }
   return 0;
}

int enter_name(MINODE * parentInode, int childInodeNum, char * childName) {
    char buffer[BLKSIZE];
    memset(buffer, '\0', BLKSIZE);
    u16 needed_length = 4*((11+strlen(childName))/4);

    int i = 0;
    for(i = 0; i < 12; i++) {
        if (parentInode->INODE.i_block[i] == 0) {
            printf("No other entries in data block...\n");
            break;
        }

        get_block(parentInode->dev, parentInode->INODE.i_block[i], buffer);
        char * cp = buffer;
        dp = (DIR *) cp;

        printf("Stepping to last entry in data block...\n");
        while(cp + dp->rec_len < buffer + BLKSIZE) {
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
