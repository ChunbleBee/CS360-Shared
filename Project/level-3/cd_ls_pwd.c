/************* cd_ls_pwd.c file **************/

extern int readlink(MINODE * linkMInode);

int chdir(char *pathname) { // cd
    printf("chdir %s\n", pathname);
    // printf("under construction READ textbook HOW TO chdir!!!!\n");
    // READ Chapter 11.7.3 HOW TO chdir
    int inode = getino(pathname);

    MINODE * min = iget(dev, inode);

    if (S_ISDIR(min->INODE.i_mode)) {
        iput(running->cwd);
        running->cwd = min;
    } else {
        printf("Failure: [ %s ] Not a directory!\n", pathname);
        iput(min);
        dev = running->cwd->dev;
    }
}

int ls_file(MINODE *mip, char *name)
{
  // printf("ls_file: to be done: READ textbook for HOW TO!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  char type, perm[10] = "wrxwrxwrx";
  __u16 mode = mip->INODE.i_mode;
  if (S_ISDIR(mode)) type = 'd';
  else if (S_ISLNK(mode)) type = 'l';
  else type = '-';
  for (int i = 0; i < 9; i++) if (!(mode & (1 << i))) perm[8 - i] = '-';
  __u16 links = mip->INODE.i_links_count;
  __u16 owner = mip->INODE.i_uid;
  __u16 group = mip->INODE.i_gid;
  time_t date = mip->INODE.i_mtime;
  __u32 size = mip->INODE.i_size;
  printf("%c%s% 4d% 4d% 4d  %.20s % 8d    %s",
    type, perm, links, owner, group, ctime(&date)+4, size, name);
  if (S_ISLNK(mode)) {
    int linkNameLen = readlink(mip);
    printf(" -> %.*s", linkNameLen, linkedNameBuffer);
  }
  printf("\n");
}

int ls_dir(MINODE *mip) {
    // printf("ls_dir: list CWD's file names; YOU do it for ls -l\n");

    u8 buf[BLKSIZE], temp[256];
    DIR *dp;
    u8 *cp;
    for(int i = 0; i < 12; i++) {
        if (mip->INODE.i_block[i] == 0) break;
    
        get_block(mip->dev, mip->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        while (cp < buf + BLKSIZE) {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            // printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
            MINODE * inode = iget(mip->dev, dp->inode);
            // printf("Record Length: %u\t", dp->rec_len);
            ls_file(inode, temp);
            iput(inode);
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("\n");
    }
}

int ls(char *pathname) {
    printf("ls %s\n", pathname);
    //printf("ls CWD only! YOU do it for ANY pathname\n");
    if (pathname[0] != '\0') {
        int inode_number = getino(pathname);
        MINODE * min = iget(dev, inode_number);
        if (S_ISDIR(min->INODE.i_mode)) {
            ls_dir(min);
        } else {
            printf("Failure: [ %s ] Not a directory!\n", pathname);
        }
        if (min != NULL) iput(min);
    } else {
        dev = running->cwd->dev;
        ls_dir(running->cwd);
    }
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

void recursivePWD(MINODE * cur_minode) {
    if ((cur_minode->ino != root->ino) ||
        (cur_minode->dev != root->dev)
    ) {
        int cur_inum = cur_minode->ino;
        int parent_inum = getino("..");
        MINODE * parent_minode = iget(dev, parent_inum);
        char cur_name[256];
        if (parent_minode->dev != cur_minode->dev) {
            MTABLE * p_mtable;
            int i;
            for (i = 0; i < NMTABLE; i++) {
                p_mtable = &mtable[i];
                if (p_mtable->dev == cur_minode->dev) {
                    break;
                }
            }
            if (i == NMTABLE) {
                printf("ERROR IN pwd(): broken mount table!\n");
                exit(3);
            }
            findmyname(parent_minode, p_mtable->mptr->ino, cur_name);
        } else {
            findmyname(parent_minode, cur_inum, cur_name);
        }
        running->cwd = parent_minode;
        recursivePWD(parent_minode);
        iput(parent_minode);
        printf("/%s", cur_name);
    }
}

void pwd(MINODE *wd) {
    MINODE * cwd_ref = running->cwd;
    printf("CWD = ");
    if (wd == root) printf("/");
    recursivePWD(wd);
    printf("\n");
    running->cwd = cwd_ref;
}
