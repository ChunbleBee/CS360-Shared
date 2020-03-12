/************* cd_ls_pwd.c file **************/

int chdir(char *pathname) {
    printf("chdir %s\n", pathname);
    printf("under construction READ textbook HOW TO chdir!!!!\n");
    // READ Chapter 11.7.3 HOW TO chdir
    int inode = getino(pathname);

    MINODE * min = iget(dev, inode);

    if (S_ISDIR(min->INODE.i_mode)) {
        iput(running->cwd);
        running->cwd = min;
    } else {
        printf("Failure: [ %s ] Not a directory!\n", pathname);
    }
}

int ls_file(MINODE *mip, char *name)
{
  printf("ls_file: to be done: READ textbook for HOW TO!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
}

int ls_dir(MINODE *mip)
{
  printf("ls_dir: list CWD's file names; YOU do it for ls -l\n");

  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // Assume DIR has only one data block i_block[0]
  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;

     printf("[%d %s]  ", dp->inode, temp); // print [inode name]

     cp += dp->rec_len;
     dp = (DIR *)cp;
  }
  printf("\n");
}

int ls(char *pathname)
{
  printf("ls %s\n", pathname);
  printf("ls CWD only! YOU do it for ANY pathname\n");
  ls_dir(running->cwd);
}

/*************** Algorithm of pwd ***************
 *  rpwd( MINODE *wd){
 *      (1). if (wd == root) return;
 *      (2). from wd->INODE.i_block[0], get my_ino and parent_ino
 *      (3). pip = iget(dev, parent_ino);
 *      (4). from pip->INODE.i_block[]: get my_name string by my_ino as LOCAL
 *      (5). rpwd(pip);
 *      // recursive call rpwd( pip) with parent minode
 */

char *pwd(MINODE *wd){
    recursivePWD(wd);
    printf("\n");
}

void recursivePWD(MINODE *curNode) {
    if (curNode == root) {
        printf("/");
    } else {
        int parentINode = findino(curNode, curNode->INODE.i_block[0]);
        MINODE parent = iget(dev, parentINode);
        
        printf("/%s", childNode.name);
        recursivePWD(parent);
    }
}
