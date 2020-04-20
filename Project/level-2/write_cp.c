int tryWrite(int fileDesc, u8 buffer[], u32 bytesProvided);
int writeToFile(OFT * file, u8 writeBuffer[], u32 bytesProvided);
int tryCopy(char * source, char * destination);
int copyFile(OFT * source, OFT * destination);
int tryCopy(char * source, char * destination);

int tryWrite(int fileDesc, u8 buffer[], u32 bytesProvided) {
    if (running->fd[fileDesc] == NULL) {
        printf("Error: file is not open!\n");
        return -1;
    }

    OFT * file = running->fd[fileDesc];

    if (file->mode == READ_MODE) {
        printf("Error: file not opened in write mode!\n");
        return -2;
    }

    return writeToFile(file, buffer, bytesProvided);
}

int writeToFile(OFT * file, u8 writeBuffer[], u32 bytesProvided) {
    MINODE * fileMInode = file->mptr;
    u8 blockBuffer[BLKSIZE + 1];
    int * iBuffer = (int *) blockBuffer;
    blockBuffer[BLKSIZE] = 0;
    int bytesWrote = 0;

    while (bytesProvided > 0) {
        int logicalBlock = file->offset / BLKSIZE;
        int startingByte = file->offset % BLKSIZE;
        int remainingBytesInBlock = BLKSIZE - startingByte;
        int physicalBlock;

        if (logicalBlock < 12) {
            if (fileMInode->INODE.i_block[logicalBlock] == 0) {
                fileMInode->INODE.i_block[logicalBlock] =
                    balloc(fileMInode->dev);  
                if (fileMInode->INODE.i_block[logicalBlock] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                fileMInode->dirty = 1;
                fileMInode->INODE.i_blocks += 2;
            }

            physicalBlock = fileMInode->INODE.i_block[logicalBlock];
        } else if ((logicalBlock < (256 + 12)) && logicalBlock >= 12) {
            if (fileMInode->INODE.i_block[12] == 0) {
                fileMInode->INODE.i_block[12] = balloc(fileMInode->dev);
                if (fileMInode->INODE.i_block[12] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                fileMInode->dirty = 1;
                fileMInode->INODE.i_blocks += 2;
            }
            printf("In single indirect. Searching for logical block: %d\n", logicalBlock);
            get_block(fileMInode->dev, fileMInode->INODE.i_block[12], ((u8 *) iBuffer));
            if (iBuffer[logicalBlock - 12] == 0) {
                iBuffer[logicalBlock - 12] = balloc(fileMInode->dev);
                if (iBuffer[logicalBlock - 12] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                put_block(fileMInode->dev, fileMInode->INODE.i_block[12], ((u8 *) iBuffer));
                fileMInode->dirty = 1;
                fileMInode->INODE.i_blocks += 2;
            }
            physicalBlock = iBuffer[(logicalBlock - 12)];

        } else if (logicalBlock < 65804) {
            if (fileMInode->INODE.i_block[13] == 0) {
                fileMInode->INODE.i_block[13] = balloc(fileMInode->dev);
                if (fileMInode->INODE.i_block[12] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
            }
            get_block(fileMInode->dev, fileMInode->INODE.i_block[13],
                (u8 *) iBuffer);
            printf("In double indirect. Searching for logical block: %d\n", logicalBlock);
            if (iBuffer[(logicalBlock - 256 - 12) / 256] == 0) {
                iBuffer[(logicalBlock - 256 - 12) / 256] =
                    balloc(fileMInode->dev);
                if (iBuffer[(logicalBlock - 256 - 12) / 256] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                put_block(fileMInode->dev, fileMInode->INODE.i_block[13],
                    (u8 *) iBuffer);
                    
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
            }
            int iBlock = iBuffer[(logicalBlock - 12) / 256];
            get_block(fileMInode->dev, iBlock, (u8 *) iBuffer);
            if (iBuffer[(logicalBlock - 256 - 12) % 256] == 0) {
                iBuffer[(logicalBlock - 256 - 12) % 256] =
                    balloc(fileMInode->dev);
                if (iBuffer[(logicalBlock - 256 - 12) % 256] == 0) {
                    printf("Write error: disk full/n");
                    return -12;
                }
                put_block(fileMInode->dev, iBlock, (u8 *) iBuffer);
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
            }
            physicalBlock = iBuffer[(logicalBlock - 256 - 12) % 256];
        } else {
            printf(
              "Damn it, Jim - we can't handle Triple-Indirect Blocks yet!!!\n");
            return -1;
        }

        get_block(fileMInode->dev, physicalBlock, blockBuffer);
        int numBytesToWrite = min(remainingBytesInBlock, bytesProvided);
        memcpy (
            &(blockBuffer[startingByte]),
            &(writeBuffer[bytesWrote]), 
            numBytesToWrite
        );
        put_block(fileMInode->dev, physicalBlock, blockBuffer);
        bytesWrote += numBytesToWrite;
        bytesProvided -= numBytesToWrite;
        file->offset += numBytesToWrite;
        printf("Wrote %d bytes to logical block: %d, physical block: %d\n", numBytesToWrite, logicalBlock, physicalBlock);

        //memcpy(blockBuffer, '\0', BLKSIZE + 1);
        // get_block(fileMInode->dev, physicalBlock, blockBuffer);
        // printf("%s\n\t\t\t\t\t\t-----------------------", blockBuffer);
    }
    printf("Wrote: %d total bytes\n", bytesWrote);
    fileMInode->INODE.i_size += bytesWrote;
    fileMInode->dirty = 1;
    return bytesWrote;
}

int tryCopy(char * sourcePath, char * destPath) {
    if (sourcePath[0] == '/') {
        dev = root->dev;
    } else {
        dev = running->cwd->dev;
    }
    int inodeNum = getino(sourcePath);
    int sourceFD;
    if (inodeNum == 0) {
        printf("copy failed: %s not found\n", sourcePath);
        return -3;
    }  
    sourceFD = open_file(sourcePath, READ_MODE);
    if (sourceFD < 0) {
        printf("copy failed to open %s\n", sourcePath);
        return -1;
    }
    int destFD = open_file(destPath, WRITE_MODE);
    if (destFD < 0) {
        printf("copy failed to open %s\n", destPath);
        close_file(sourceFD);
        return -2;
    }
    u8 buffer[BLKSIZE + 1];
    buffer[BLKSIZE] = 0;

    int totalWrites = 0;
    int totalReads = 0;
    int numBytes = tryRead(sourceFD, buffer, BLKSIZE);
    while (numBytes > 0) {
        totalWrites++;
        tryWrite(destFD, buffer, numBytes);
        numBytes = tryRead(sourceFD, buffer, BLKSIZE);
    }
    printf("Total writes: %d\n", totalWrites);
    close_file(destFD);
    // cat(destPath);
    close_file(sourceFD);
    return 1;
}