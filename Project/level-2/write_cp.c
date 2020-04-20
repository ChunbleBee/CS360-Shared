int tryWrite(int fileDesc, u8 buffer[], u32 bytesInBuffer);
int writeToFile(OFT * file, u8 writeBuffer[], u32 bytesInBuffer);
int tryCopy(char * source, char * destination);
int copyFile(OFT * source, OFT * destination);
int tryCopy(char * source, char * destination);

int tryWrite(int fileDesc, u8 buffer[], u32 bytesInBuffer) {
    if (running->fd[fileDesc] == NULL) {
        printf("Error: file is not open!\n");
        return -1;
    }

    OFT * file = running->fd[fileDesc];

    if (file->mode == READ_MODE) {
        printf("Error: file not opened in write mode!\n");
        return -2;
    }

    return writeToFile(file, buffer, bytesInBuffer);
}

int writeToFile(OFT * file, u8 writeBuffer[], u32 bytesInBuffer) {
    MINODE * fileINode = file->mptr;
    u8 blockBuffer[BLKSIZE];
    int totBytesWrote = 0;
    printf("offset: %u, total bytes to write: %u\n", file->offset, bytesInBuffer);
    while (totBytesWrote < bytesInBuffer) {
        printf("current buffer: %s\n\tbytes left to write: %u\n", &(writeBuffer[totBytesWrote]), bytesInBuffer - totBytesWrote);
        int logicalBlock = file->offset / BLKSIZE;
        int startingByte = file->offset % BLKSIZE;
        int remainingBytesInBlock = BLKSIZE - startingByte;
        printf("lblock %d\tsbyte %d\tremaining%d\n", logicalBlock, startingByte, remainingBytesInBlock);

        int physicalBlock = 0;

        if (logicalBlock < 12) {
            printf("inode < 12\n");
            if (fileINode->INODE.i_block[logicalBlock] == 0) {
                fileINode->INODE.i_block[logicalBlock] = balloc(fileINode->dev);
            }
            
            if (fileINode->INODE.i_block[logicalBlock] != 0){
                physicalBlock = fileINode->INODE.i_block[logicalBlock];
            }
        } else if (logicalBlock >= 12 && logicalBlock < 268) {
            printf("12 >= inode > 268\n");
            u32 indirectBlockBuffer[256];
            if (fileINode->INODE.i_block[12] == 0) {
                fileINode->INODE.i_block[12] = balloc(fileINode->dev);
            }
            get_block(fileINode->dev, fileINode->INODE.i_block[12], (u8 *) indirectBlockBuffer);
            if (indirectBlockBuffer[logicalBlock - 12] == 0) {
                indirectBlockBuffer[logicalBlock - 12] = balloc(fileINode->dev);
                put_block(fileINode->dev, fileINode->INODE.i_block[12], (u8 *) indirectBlockBuffer);
            }
        } else if (logicalBlock >= 268 && logicalBlock < 65804) {
            printf("268 >= inode > 65048\n");
            u32 indirectBlockBuffer[256];
            u32 doubleIndirectBlockBuffer[256];

            if (fileINode->INODE.i_block[13] == 0) {
                fileINode->INODE.i_block[13] = balloc(fileINode->dev);
            }
            get_block(fileINode->dev, fileINode->INODE.i_block[13], (u8 *) indirectBlockBuffer);
            if (indirectBlockBuffer[(logicalBlock - 268) / 256] == 0) {
                indirectBlockBuffer[(logicalBlock - 268) / 256] = balloc(fileINode->dev);
                put_block(fileINode->dev, fileINode->INODE.i_block[13],  (u8 *) indirectBlockBuffer);
            }
            get_block(fileINode->dev, indirectBlockBuffer[(logicalBlock - 268) / 256], (u8 *) doubleIndirectBlockBuffer);
            if (doubleIndirectBlockBuffer[(logicalBlock - 268) % 256] == 0) {
                doubleIndirectBlockBuffer[(logicalBlock - 268) % 256] = balloc(fileINode->dev);
            }
            physicalBlock = doubleIndirectBlockBuffer[(logicalBlock - 268) % 256];
        } else {
            printf("Damn it, Jim - we can't handle Triple-Indirect Blocks yet!!!\n");
            return -1;
        }
        if (physicalBlock  == 0) {
            printf("~~ Panic at the Disco Mode ~~\n\tNot enough meme-ory on the disk to do diz shiz.\n");
            return -1;
        }
        get_block(fileINode->dev, physicalBlock, blockBuffer);
        int numBytesWrote = min(remainingBytesInBlock, (bytesInBuffer - totBytesWrote));
        memcpy(&(blockBuffer[startingByte]), &(writeBuffer[totBytesWrote]), numBytesWrote);
        printf("String to write to block: %s\n", blockBuffer);
        totBytesWrote += numBytesWrote;
        remainingBytesInBlock -= numBytesWrote;
        file->offset += numBytesWrote;
        put_block(fileINode->dev, physicalBlock, blockBuffer);
        
    }
    fileINode->INODE.i_size = fileINode->INODE.i_size += bytesInBuffer;
    printf("Total bytes wrote: %d, total bytes in buffer: %d\n", totBytesWrote, bytesInBuffer);
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