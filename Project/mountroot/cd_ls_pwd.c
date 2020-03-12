/************* cd_ls_pwd.c file **************/

int chdir(char *pathname)   
{
  printf("chdir %s\n", pathname);
  printf("under construction READ textbook HOW TO chdir!!!!\n");
  // READ Chapter 11.7.3 HOW TO chdir
}

int ls_file(MINODE *mip, char *name)
{
  // printf("ls_file: to be done: READ textbook for HOW TO!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  char type, perm[10] = "wrxwrxwrx";
  __u16 mode = mip->INODE.i_mode;
  if (S_ISDIR(mode)) type = 'd'; else type = '-';
  for (int i = 0; i < 9; i++) if (!(mode & (1 << i))) perm[i] = '-';
  __u16 links = mip->INODE.i_links_count;
  __u16 owner = mip->INODE.i_uid;
  __u16 group = mip->INODE.i_gid;
  time_t date = mip->INODE.i_mtime;
  __u32 size = mip->INODE.i_size;
  printf("%c%s \t%d \t%d \t%d \t%s \t% 8d \t%s\n",
    type, perm, links, owner, group, ctime(date), size, name);
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
	
     // printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
     ls_file(iget(dev, dp->inode), temp);
	 
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

char *pwd(MINODE *wd)
{
  printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return;
  }
}



