int tryRead(int fileDesc, u8 buffer[], u32 numBytes) {
    if (running->fd[fileDesc] == NULL) {
        printf("Error: File descriptor #%u does not exist!\n", fileDesc);
        return -1;
    }

    OFT * file = running->fd[fileDesc];

    if (file->mode != READ_MODE && file->mode != READ_WRITE_MODE) {
        printf("Error: file not opened in read or read/write mode!\n");
        return -2;
    }

    return readFromFile(file, buffer, numBytes);
}

int readFromFile(OFT * file, u8 readBuffer[], u32 numBytes) {
    MINODE * fileINode = file->mptr;
    int offset = file->offset;
    u32 availableBytes = file->mptr->INODE.i_size - offset;
    u8 blockBuffer[BLKSIZE];
    int readBytes = 0;

    while(numBytes && availableBytes) {
        int logicalBlock = file->offset / BLKSIZE;
        int startingByte = offset % BLKSIZE;
        int remainingBytesInBlock = BLKSIZE - startingByte;
        int physicalBlock = logicalBlock;

        if (logicalBlock >= 12 && logicalBlock < 268) {
            u32 indirectBlockBuffer[256];
            get_block(fileINode->dev, fileINode->INODE.i_block[12], (u8 *) indirectBlockBuffer);
            physicalBlock = indirectBlockBuffer[logicalBlock - 12];
        } else if (logicalBlock >= 268 && logicalBlock < 65804) {
            u32 indirectBlockBuffer[256];
            u32 doubleIndirectBlockBuffer[256];
            get_block(fileINode->dev, fileINode->INODE.i_block[13], (u8 *) indirectBlockBuffer);
            get_block(fileINode->dev, indirectBlockBuffer[(logicalBlock - 268) / 256], (u8 *) doubleIndirectBlockBuffer);
            physicalBlock = doubleIndirectBlockBuffer[(logicalBlock - 268) % 256];
        } else if (logicalBlock >= 65804) {
            printf("Error: File size out of bounds!!! D:\n\t(Triple Indirect Blocks not implemented).\n");
            return -1;
        }

        get_block(fileINode->dev, physicalBlock, blockBuffer);
        int currentByte = startingByte;

        while (remainingBytesInBlock > 0) {
            readBuffer[readBytes] = blockBuffer[currentByte];
            readBytes++;
            currentByte++;
            file->offset++;

            availableBytes--;
            numBytes--;
            remainingBytesInBlock--;
            if (numBytes <= 0 || availableBytes <= 0) {
                break;
            }
        }
    }

    return readBytes;
}

void cat(char * name, int mode) {
    u8 buffer[BLKSIZE];
    int fileDesc = open_file(name, mode);
    int bytesRead = tryRead(fileDesc, buffer, BLKSIZE - 1);

    while(bytesRead > 0) {
        buffer[bytesRead] = '\0';
        for (int i = 0; i < BLKSIZE; i++) {
            putchar(buffer[i]);
        }

        bytesRead = tryRead(fileDesc, buffer, BLKSIZE - 1);
    }
}