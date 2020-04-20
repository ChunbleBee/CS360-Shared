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
    u8 blockBuffer[BLKSIZE];
    int bytesWrote = 0;

    while (bytesProvided > 0) {
        int logicalBlock = file->offset / BLKSIZE;
        int startingByte = file->offset % BLKSIZE;
        int remainingBytesInBlock = BLKSIZE - startingByte;
        int physicalBlock;

        // int physicalBlock = logicalBlock;

        if (logicalBlock < 12) {
            if (fileMInode->INODE.i_block[logicalBlock] == 0) {
                fileMInode->INODE.i_block[logicalBlock] =
                    balloc(fileMInode->dev);  
                if (fileMInode->INODE.i_block[logicalBlock] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
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
            }
            get_block(fileMInode->dev, fileMInode->INODE.i_block[12],
                (u8 *) iBuffer);
            if (iBuffer[logicalBlock - 12] == 0) {
                iBuffer[logicalBlock - 12] = balloc(fileMInode->dev);
                if (iBuffer[logicalBlock - 12] == 0) {
                    printf("Write error: disk full\n");
                    return -12;
                }
                put_block(fileMInode->dev, fileMInode->INODE.i_block[12],
                    (u8 *) indirectBlockBuffer);
            }
        } else if (logicalBlock >= 268 && logicalBlock < 65804) {
            printf("268 >= inode > 65048\n");
            u32 indirectBlockBuffer[256];
            u32 doubleIndirectBlockBuffer[256];

            if (fileMInode->INODE.i_block[13] == 0) {
                fileMInode->INODE.i_block[13] = balloc(fileMInode->dev);
            }
            get_block(fileMInode->dev, fileMInode->INODE.i_block[13],
                (u8 *) indirectBlockBuffer);
            if (indirectBlockBuffer[(logicalBlock - 268) / 256] == 0) {
                indirectBlockBuffer[(logicalBlock - 268) / 256] =
                    balloc(fileMInode->dev);
                put_block(fileMInode->dev, fileMInode->INODE.i_block[13],
                    (u8 *) indirectBlockBuffer);
            }
            get_block(fileMInode->dev, indirectBlockBuffer[
                (logicalBlock - 268) / 256], (u8 *) doubleIndirectBlockBuffer);
            if (doubleIndirectBlockBuffer[(logicalBlock - 268) % 256] == 0) {
                doubleIndirectBlockBuffer[(logicalBlock - 268) % 256] =
                    balloc(fileMInode->dev);
            }
            physicalBlock =
                doubleIndirectBlockBuffer[(logicalBlock - 268) % 256];
        } else {
            printf(
              "Damn it, Jim - we can't handle Triple-Indirect Blocks yet!!!\n");
            return -1;
        }

        get_block(fileMInode->dev, physicalBlock, blockBuffer);
        int numBytesToWrite = min(remainingBytesInBlock, bytesProvided);
        memcpy(&(blockBuffer[startingByte]), &(writeBuffer[bytesWrote]), 
            numBytesToWrite);
        bytesWrote += numBytesToWrite;
        bytesProvided -= numBytesToWrite;
        remainingBytesInBlock -= numBytesToWrite;
        file->offset += numBytesToWrite;
        put_block(fileMInode->dev, physicalBlock, blockBuffer);
        
    }
    fileINode->INODE.i_size = fileINode->INODE.i_size += bytesInBuffer;
    printf("Total bytes wrote: %d, total bytes in buffer: %d\n",
        totBytesWrote, bytesInBuffer);
    return totBytesWrote;
}

int tryCopy(char * sourcePath, char * destPath) {
    int sourceFD = open_file(sourcePath, READ_MODE);

    if (sourceFD >= 0) {
        if (tryCreate(destPath) == 1) {
            int destFD = open_file(destPath, WRITE_MODE);

            if (destFD >= 0) {
                u8 buffer[BLKSIZE];
                int numBytes = tryRead(sourceFD, buffer, BLKSIZE);

                while (numBytes > 0) {
                    tryWrite(destFD, buffer, numBytes);
                    numBytes = tryRead(sourceFD, buffer, BLKSIZE);
                }

                close_file(destFD);
            }
        }

        close_file(sourceFD);
    }
}