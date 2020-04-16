int tryRead(int fileDesc, u8 buffer[], u32 numBytes);
int readFromFile(OFT * file, u8 readBuffer[], u32 numBytes);
void cat(char * name);

int tryRead(int fileDesc, u8 buffer[], u32 numBytes) {
    if (running->fd[fileDesc] == NULL) {
        printf("Error: File is not open!\n");
        return -1;
    }

    OFT * file = running->fd[fileDesc];

    if (file->mode != READ_MODE && file->mode != READ_WRITE_MODE) {
        printf("Error: file not opened in read mode!\n");
        return -2;
    }

    return readFromFile(file, buffer, numBytes);
}

int readFromFile(OFT * file, u8 readBuffer[], u32 numBytes) {
    MINODE * fileMInode = file->mptr;
    u32 availableBytes = fileMInode->INODE.i_size - file->offset;
    u8 blockBuffer[BLKSIZE];
    int bytesRead = 0;

    while (numBytes && availableBytes) {
        int logicalBlock = file->offset / BLKSIZE;
        int startingByte = file->offset % BLKSIZE;
        int remainingBytesInBlock = BLKSIZE - startingByte;
        int physicalBlock = logicalBlock;

        if (logicalBlock >= 12 && logicalBlock < 268) {
            u32 indirectBlockBuffer[256];
            get_block(fileMInode->dev, fileMInode->INODE.i_block[12], (u8 *) indirectBlockBuffer);
            physicalBlock = indirectBlockBuffer[logicalBlock - 12];
        } else if (logicalBlock >= 268 && logicalBlock < 65804) {
            u32 indirectBlockBuffer[256];
            u32 doubleIndirectBlockBuffer[256];
            get_block(fileMInode->dev, fileMInode->INODE.i_block[13], (u8 *) indirectBlockBuffer);
            get_block(fileMInode->dev, indirectBlockBuffer[(logicalBlock - 268) / 256], (u8 *) doubleIndirectBlockBuffer);
            physicalBlock = doubleIndirectBlockBuffer[(logicalBlock - 268) % 256];
        } else if (logicalBlock >= 65804) {
            printf("Error: File size out of bounds!!! D:\n\t(Triple Indirect Blocks not implemented).\n");
            return -1;
        }
        physicalBlock++;

        get_block(fileMInode->dev, physicalBlock, blockBuffer);
        int numBytesToRead = min(min(availableBytes, remainingBytesInBlock), numBytes);
        memcpy(&(readBuffer[bytesRead]), &(blockBuffer[startingByte]), numBytesToRead);
        bytesRead += numBytesToRead;
        availableBytes -= numBytesToRead;
        numBytes -= numBytesToRead;
        remainingBytesInBlock -= numBytesToRead;
        file->offset += numBytesToRead;
        // while (remainingBytesInBlock > 0) {
        //     readBuffer[bytesRead] = blockBuffer[currentByte];
        //     bytesRead++;
        //     currentByte++;
        //     file->offset++;

        //     availableBytes--;
        //     numBytes--;
        //     remainingBytesInBlock--;
        //     if (numBytes <= 0 || availableBytes <= 0) {
        //         break;
        //     }
        // }
    }

    return bytesRead;
}

void cat(char * name) {
    u8 buffer[BLKSIZE + 1];
    int fileDesc = open_file(name, READ_MODE);

    if (fileDesc >= 0) {
        int bytesRead = tryRead(fileDesc, buffer, BLKSIZE);
        putchar('\n');

        while (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            puts(buffer);
            bytesRead = tryRead(fileDesc, buffer, BLKSIZE);
        }

        close_file(fileDesc);
    }
}