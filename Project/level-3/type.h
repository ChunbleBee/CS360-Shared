/*************** type.h file ************************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE  0
#define READY 1

#define BLKSIZE           1024
#define NMINODE            128
#define NFD                 16
#define NPROC               20
#define NOFT     (NFD * NPROC)
#define NMTABLE             16

#define READ_MODE       0
#define WRITE_MODE      1
#define READ_WRITE_MODE 2
#define APPEND_MODE     3

#define ROOT_GROUP    0
#define USER_GROUP    1
#define OTHER_GROUP   2
#define ERROR_GROUP   3

typedef struct mtable {
  int dev;
  struct minode * mptr;
  char name[64];
  int ninodes;
  int nblocks;
  int imap;
  int bmap;
  int inode_start;
} MTABLE;

typedef struct minode {
  INODE INODE;
  int dev, ino;
  int refCount;
  int dirty;
  int mounted;
  int mode;
  MTABLE *mptr;
} MINODE;

typedef struct oft {
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
} OFT;

typedef struct proc {
  struct proc *next;
  int          pid;
  int          status;
  int          uid, gid;
  MINODE      *cwd;
  OFT         *fd[NFD];
} PROC;
