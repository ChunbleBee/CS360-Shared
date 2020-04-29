
int printMTables () {
    printf("mounted disks:\n");
    printf("    dev    name\n")
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
    }
    printf("checking EXT2 FS ....");
    int fileDesc = open(diskname, O_RDWR);
    if (fileDesc < 0) {
        printf("open %s failed\n", diskname);
        return -2;
    }
    u8 * buffer[BLKSIZE];
    SUPER * p_superblock = getblock(fileDesc, 1, buffer);
    if (p_superblock->s_magic != 0xEF53) {
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        umount(diskname);
        return -3;
    }
    int ino = getino(mountpoint);
    MINODE * mip = iget(dev, ino);
    if (S_ISDIR(mip->INODE.i_mode) == 0) {
        printf("mountpoint: %s is not a directory\n", mountpoint);
        iput(mip);
        return -4;
    }
    for (i = 0; i < NPROC; i++) {
        if (proc[i].cwd->ino == mip->ino) {
            printf("mountpoint: %s is currently in use\n", mountpoint);
            iput(mip);
            return -5;
        }
    }
    p_mtable->dev = fileDesc;
    // todo: lots
}


