
int printMTables () {
    printf("mounted disks:\n");
    printf("    dev     ninodes nblocks imap    bmap    inode_start   name\n");
    MTABLE * p_mtable;
    for (int i = 0; i < NMTABLE; i++) {
        p_mtable = &mtable[i];
        if (p_mtable->dev != 0) {
            printf("    %6d  %6d  %6d  %6d  %6d  %6d      %s\n",
                p_mtable->dev,
                p_mtable->ninodes,
                p_mtable->nblocks,
                p_mtable->imap,
                p_mtable->bmap,
                p_mtable->inode_start,
                p_mtable->name);
        }
    }
}

int mount (char * diskname, char * mountpoint) {
    MTABLE * p_mtable;
    int i;
    for (i = 0; i < NMTABLE; i++) {
        p_mtable = &mtable[i];
        if (strcmp(p_mtable->name, diskname) == 0) {
            printf("disk: %s is already mounted!\n", diskname);
            return -1;
        }
    }
    printf("disk: %s is not currently mounted\n", diskname);
    for (i = 0; i < NMTABLE; i++) {
        p_mtable = &mtable[i];
        if (p_mtable->dev == 0) {
            printf("allocating mtable[%d]\n", i);
            break;
        }
    }
    if (i == NMTABLE) {
        printf("all mtables are in use\n");
        return -2;
    } else {
        printf("allocated mtable[%d] for mounting", i);
    }
    printf("checking EXT2 FS ....");
    int fileDesc = open(diskname, O_RDWR);
    if (fileDesc < 0) {
        printf("open %s failed\n", diskname);
        return -2;
    } else {
        printf("opened %s for read/write\n", diskname);
    }
    u8 buffer[BLKSIZE];
    get_block(fileDesc, 1, buffer);
    SUPER * p_superblock = (SUPER *) buffer;
    if (p_superblock->s_magic != 0xEF53) {
        printf("magic = %x is not an ext2 filesystem\n", p_superblock->s_magic);
        return -3;
    } else {
        printf("%s is an ext2 file system\n", diskname);
    }
    int ino = getino(mountpoint);
    MINODE * mip = iget(dev, ino);
    if (S_ISDIR(mip->INODE.i_mode) == 0) {
        printf("mountpoint: %s is not a directory\n", mountpoint);
        iput(mip);
        return -4;
    } else {
        printf("mountpoint: %s is a directory\n", mountpoint);
    }
    for (i = 0; i < NPROC; i++) {
        if ((proc[i].cwd != NULL) && (proc[i].cwd->ino == mip->ino)) {
            printf("mountpoint: %s is currently in use\n", mountpoint);
            iput(mip);
            return -5;
        }
    }
    printf("mountpoint is avalible for use - initializing\n");
    p_mtable->dev = fileDesc;
    strncpy(p_mtable->name, diskname, 64);
    p_mtable->ninodes = p_superblock->s_inodes_count;
    p_mtable->nblocks = p_superblock->s_blocks_count;
    get_block(p_mtable->dev, 2, buffer);
    GD * p_groupdesc = (GD *) buffer;
    p_mtable->bmap = p_groupdesc->bg_block_bitmap;
    p_mtable->imap = p_groupdesc->bg_inode_bitmap;
    p_mtable->inode_start = p_groupdesc->bg_inode_table;
    mip->mounted = 1;
    mip->mptr = p_mtable;
    p_mtable->mptr = mip;

    return 1;
}

int umount(char * diskname) {
    MTABLE * p_mtable;
    int i;
    for (i = 0; i < NMTABLE; i++) {
        p_mtable = &mtable[i];
        if (strcmp(p_mtable->name, diskname) == 0) {
            printf("found disk: %s mounted in mtable[%d]\n", diskname, i);
            break;
        }
    }
    if (i == NMTABLE) {
        printf("disk: %s not found in mount tables\n", diskname);
        return -1;
    }
    for (i = 0; i < NPROC; i++) {
        if (proc[i].cwd != NULL &&
            proc[i].cwd->dev == p_mtable->dev
        ) {
            printf("disk: %s is currently in use in proc %d\n", diskname, i);
            return -2;
        }
    }
    for (i = 0; i < NMINODE; i++) {
        if ((minode[i].dev == p_mtable->dev) &&
            (p_mtable->mptr->ino != minode[i].ino ||
                p_mtable->mptr->refCount > 1)
        ) {
            printf("disk: %s is currently in use - opended as minode[%d]\n",
                diskname, i);
            return -3;
        }
    }
    p_mtable->mptr->mounted = 0;
    iput(p_mtable->mptr);
    close(p_mtable->dev);
    p_mtable->dev = 0;
    p_mtable->name[0] = '\0';
    p_mtable->ninodes = 0;
    p_mtable->nblocks = 0;
    p_mtable->bmap = 0;
    p_mtable->imap = 0;
    p_mtable->inode_start = 0;
    p_mtable->mptr = NULL;
    printf("Successfully unmounted %s\n", diskname);
    return 1;
}
