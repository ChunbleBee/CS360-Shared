int tryWrite(int fileDesc, u8 buffer[], u32 numBytes);
int writeToFile(OFT * file, u8 writeBuffer[], u32 numBytes);
int tryCopy(char * source, char * destination);
int copyFile(OFT * source, OFT * destination);
int tryMove(char * source, char * destination);
int moveFile(OFT * source, OFT * destination);

int tryWrite(int fileDesc, u8 buffer[], u32 numBytes) {
    if (running->fd[fileDesc] == NULL) {
        print("Error: file is not open!\n");
        return -1;
    }

    OFT * file = running->fd[fileDesc];

    if (file->mode != WRITE_MODE && file->mode != READ_WRITE_MODE) {
        printf("Error: file not opened in write mode!\n");
        return -2;
    }

    return writeToFile(file, buffer, numBytes);
}

int writeToFile(OFT * file, u8 writeBuffer, u32 numBytes) {
    MINODE * fileINode = file->mptr;
    int offset = file->offset;
    u8 blockBuffer[BLKSIZE];
    int bytesWrote = 0;

    while (numBytes > 0) {
        int logicalBlock = file->offset / BLKSIZE;
        int startingByte = offset % BLKSIZE;
        int remainingBytesInBlock = BLKSIZE - startingByte;

        int physicalBlock = logicalBlock;

        if (logicalBlock < 12) {
            if (fileINode->INODE.i_block[logicalBlock] == 0) {
                fileINode->INODE.i_block[logicalBlock] = balloc(fileINode->dev);
            }
            physicalBlock = fileINode->INODE.i_block[logicalBlock];
        } else if (logicalBlock >= 12 && logicalBlock < 268) {
            u32 indirectBlockBuffer[256];
            if (fileINode->INODE.i_block[12] == 0) {
                fileINode->INODE.i_block[12] = balloc(fileINode->dev);
            }
            get_block(fileINode->dev, fileINode->INODE.i_block[12], (u8 *) indirectBlockBuffer);
            if (indirectBlockBuffer[logicalBlock - 12] == 0) {
                indirectBlockBuffer[logicalBlock - 12] = balloc(fileINode->dev);
                put_block(fileINode->dev, fileINode->INODE.i_block[12], indirectBlockBuffer);
            }
        } else if (logicalBlock >= 268 && logicalBlock < 65804) {
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

        get_block(fileINode->dev, physicalBlock, blockBuffer);
        int numBytesToWrite = min(remainingBytesInBlock, numBytes);
        memcpy(&(blockBuffer[startingByte]), &(writeBuffer[bytesRead]), numBytesToWrite);
        bytesWrote += numBytesToWrite;
        numBytes -= numBytesToWrite;
        remainingBytesInBlock -= numBytesToRead;
        file->offset += numBytesToRead;
        put_block(fileINode->dev, physicalBlock, blockBuffer);
        
    }

    return bytesWrote;
}