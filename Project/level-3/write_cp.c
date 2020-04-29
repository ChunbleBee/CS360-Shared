int tryWrite(int fileDesc, u8 * buffer, u32 bytesProvided);
int writeToFile(OFT * file, u8 * writeBuffer, u32 bytesProvided);
int tryCopy(char * source, char * destination);
int copyFile(OFT * source, OFT * destination);
int tryCopy(char * source, char * destination);

int tryWrite(int fileDesc, u8 * buffer, u32 bytesProvided) {
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

int writeToFile(OFT * file, u8 * writeBuffer, u32 bytesProvided) {
    MINODE * fileMInode = file->mptr;
    u8 blockBuffer[BLKSIZE];
    int bytesWrote = 0;
    while (bytesProvided > 0) {
        int logicalBlock = file->offset / BLKSIZE;
        int startingByte = file->offset % BLKSIZE;
        int remainingBytesInBlock = BLKSIZE - startingByte;
        int physicalBlock;
        printf("- - - - - - - - - - - - - - - - - - - -\n");
        if (logicalBlock < 12) {
            if (fileMInode->INODE.i_block[logicalBlock] == 0) {
                fileMInode->INODE.i_block[logicalBlock] =
                    balloc(fileMInode->dev);  
                if (fileMInode->INODE.i_block[logicalBlock] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                printf("- - - - allocated block %4d as i_block[%d] - - - -\n",
                    fileMInode->INODE.i_block[logicalBlock], logicalBlock);
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
            }
            physicalBlock = fileMInode->INODE.i_block[logicalBlock];
        } else if (logicalBlock < 256 + 12) {
            int * iBuffer = (int *) blockBuffer;
            if (fileMInode->INODE.i_block[12] == 0) {
                fileMInode->INODE.i_block[12] = balloc(fileMInode->dev);
                if (fileMInode->INODE.i_block[12] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                printf("- - - - allocated block %4d as i_block[12] - - - -\n",
                    fileMInode->INODE.i_block[12]);
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
            }
            get_block(fileMInode->dev, fileMInode->INODE.i_block[12],
                (u8 *) iBuffer);
            if (iBuffer[logicalBlock - 12] == 0) {
                iBuffer[logicalBlock - 12] = balloc(fileMInode->dev);
                if (iBuffer[logicalBlock - 12] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                printf(
                    "- - - - allocated block %4d as i_block[12][%d] - - - -\n",
                    fileMInode->INODE.i_block[12], logicalBlock - 12);
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
                put_block(fileMInode->dev, fileMInode->INODE.i_block[12],
                    (u8 *) iBuffer);
            }
            physicalBlock = iBuffer[logicalBlock - 12];
        } else if (logicalBlock < 256*256 + 256 + 12) {
            int * iBuffer = (int *) blockBuffer;
            if (fileMInode->INODE.i_block[13] == 0) {
                fileMInode->INODE.i_block[13] = balloc(fileMInode->dev);
                if (fileMInode->INODE.i_block[13] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                printf("- - - - allocated block %4d as i_block[13] - - - -\n",
                    fileMInode->INODE.i_block[13]);
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
            }
            get_block(fileMInode->dev, fileMInode->INODE.i_block[13],
                (u8 *) iBuffer);
            if (iBuffer[(logicalBlock - 256 - 12) / 256] == 0) {
                iBuffer[(logicalBlock - 256 - 12) / 256] =
                    balloc(fileMInode->dev);
                if (iBuffer[(logicalBlock - 256 - 12) / 256] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                printf(
                    "- - - - allocated block %4d as i_block[13][%d] - - - -\n",
                    iBuffer[(logicalBlock - 256 - 12) / 256],
                    (logicalBlock - 256 - 12) / 256);
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
                put_block(fileMInode->dev, fileMInode->INODE.i_block[13],
                    (u8 *) iBuffer);
            }
            int iBlock = iBuffer[(logicalBlock - 256 - 12) / 256];
            get_block(fileMInode->dev, iBlock, (u8 *) iBuffer);
            if (iBuffer[(logicalBlock - 256 - 12) % 256] == 0) {
                iBuffer[(logicalBlock - 256 - 12) % 256] =
                    balloc(fileMInode->dev);
                if (iBuffer[(logicalBlock - 256 - 12) % 256] == 0) {
                    printf("Write error: disk full/n");
                    return -12;
                }
                printf(
                 "- - - - allocated block %4d as i_block[13][%d][%d] - - - -\n",
                    iBuffer[(logicalBlock - 256 - 12) % 256],
                    (logicalBlock - 256 - 12) / 256,
                    (logicalBlock - 256 - 12) % 256);
                fileMInode->INODE.i_blocks += 2;
                fileMInode->dirty = 1;
                put_block(fileMInode->dev, iBlock, (u8 *) iBuffer);
            }
            physicalBlock = iBuffer[(logicalBlock - 256 - 12) % 256];
        } else {
            printf("- - - - Damn it, Jim - ");
            printf("we can't handle Triple-Indirect Blocks yet!!! - - - -\n");
            return -1;
        }
        get_block(fileMInode->dev, physicalBlock, blockBuffer);
        int numBytesToWrite = min(remainingBytesInBlock, bytesProvided);
        memcpy(&(blockBuffer[startingByte]), &(writeBuffer[bytesWrote]), 
            numBytesToWrite);
        
        bytesWrote += numBytesToWrite;
        bytesProvided -= numBytesToWrite;
        remainingBytesInBlock -= numBytesToWrite; // unused - reset in next loop
        file->offset += numBytesToWrite;
        if (file->offset > fileMInode->INODE.i_size) {
            fileMInode->INODE.i_size = file->offset;
            fileMInode->dirty = 1;
        }
        put_block(fileMInode->dev, physicalBlock, blockBuffer);
        printf("- - - - Wrote %4d bytes to block : ", numBytesToWrite);
        printf("%4d starting from byte %d in source and %d in dest - - - -\n",
            physicalBlock, bytesWrote - numBytesToWrite, startingByte);
        write(1, &(blockBuffer[startingByte]), numBytesToWrite);
    }
    printf("\n- - - - Wrote %d total bytes - - - -\n\n", bytesWrote);
    return bytesWrote;
}

int tryCopy(char * sourcePath, char * destPath) {
    int inodeNum = getino(sourcePath);
    int sourceFD;
    if (inodeNum == 0) {
        printf("copy failed: %s not found\n", sourcePath);
        return -3;
    }  
    sourceFD = open_file(sourcePath, READ_MODE);
    if (sourceFD < 0) {
        printf("copy failed to open %s for reading\n", sourcePath);
        return -1;
    }
    int destFD = open_file(destPath, WRITE_MODE);
    if (destFD < 0) {
        printf("copy failed to open %s for writing\n", destPath);
        close_file(sourceFD);
        return -2;
    }
    u8 buffer[BLKSIZE];
    int totalBytesWritten = 0;
    int numBytes = tryRead(sourceFD, buffer, BLKSIZE);
    while (numBytes > 0) {
        tryWrite(destFD, buffer, numBytes);
        totalBytesWritten += numBytes;
        printf("= = = = wrote %d bytes from %s to %s : = = = =\n",
            numBytes, sourcePath, destPath);
        write(1, buffer, numBytes);
        printf("\n= = = = = = = = = = = = = = = = = = = =\n");
        numBytes = tryRead(sourceFD, buffer, BLKSIZE);
    }
    printf("%d bytes copied from %s to %s\n",
        totalBytesWritten, sourcePath, destPath);
    printf("Wrote to byte number %d\n", running->fd[destFD]->offset);
    printBlockList(running->fd[destFD]->mptr);
    close_file(destFD);
    close_file(sourceFD);
    return 1;
}
