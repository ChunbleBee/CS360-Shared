int tryRead(int fileDesc, u8 buffer[], u32 bytesRequested);
int readFromFile(OFT * file, u8 readBuffer[], u32  bytesRequested);
void cat(char * name);

int tryRead(int fileDesc, u8 buffer[], u32  bytesRequested) {
    if (running->fd[fileDesc] == NULL) {
        printf("Error: File is not open!\n");
        return -1;
    }
    OFT * file = running->fd[fileDesc];
    if (file->mode != READ_MODE && file->mode != READ_WRITE_MODE) {
        printf("Error: file not opened in read mode!\n");
        return -2;
    }
    return readFromFile(file, buffer,  bytesRequested);
}

int readFromFile(OFT * file, u8 readBuffer[], u32 bytesRequested) {
    MINODE * fileMInode = file->mptr;
    u32 availableBytes = fileMInode->INODE.i_size - file->offset;
    u8 blockBuffer[BLKSIZE];
    int bytesRead = 0;
    while (bytesRequested && availableBytes) {
        int logicalBlock = file->offset / BLKSIZE;
        int startingByte = file->offset % BLKSIZE;
        int remainingBytesInBlock = BLKSIZE - startingByte;
        int physicalBlock;
        if (logicalBlock < 12) {
            physicalBlock = fileMInode->INODE.i_block[logicalBlock];
        } else if (logicalBlock < 256 + 12) {
            int * iBuffer = (int *) blockBuffer;
            get_block(fileMInode->dev, fileMInode->INODE.i_block[12], 
                (u8 *) iBuffer);
            physicalBlock = iBuffer[logicalBlock - 12];
        } else if (logicalBlock < 256*256 + 256 + 12) {
            int * iBuffer = (int *) blockBuffer;
            get_block(fileMInode->dev, fileMInode->INODE.i_block[13],
                (u8 *) iBuffer);
            int iBlock = iBuffer[(logicalBlock - 256 - 12) / 256];
            get_block(fileMInode->dev, iBlock, (u8 *) iBuffer);
            physicalBlock = iBuffer[(logicalBlock - 256 - 12) % 256];
        } else {
            printf("Error: File size out of bounds!!! D:\n");
            printf("    (Triple Indirect Blocks not implemented).\n");
            return -1;
        }
        get_block(fileMInode->dev, physicalBlock, blockBuffer);
        int numBytesToRead = min(min(
            availableBytes,
            remainingBytesInBlock),
            bytesRequested);
        memcpy(readBuffer, &(blockBuffer[startingByte]), numBytesToRead);
        bytesRead += numBytesToRead;
        availableBytes -= numBytesToRead;
        bytesRequested -= numBytesToRead;
        remainingBytesInBlock -= numBytesToRead;
        file->offset += numBytesToRead;
    }
    return bytesRead;
}

void cat(char * name) {
    u8 buffer[BLKSIZE];
    int fileDesc = open_file(name, READ_MODE);
    if (fileDesc >= 0) {
        int totalBytesRead = 0;
        int bytesRead = tryRead(fileDesc, buffer, BLKSIZE);
        printf("\n%s contains %u bytes\n", name,
            running->fd[fileDesc]->mptr->INODE.i_size);
        while (bytesRead > 0) {
            write(1, buffer, bytesRead);
            totalBytesRead += bytesRead;
            bytesRead = tryRead(fileDesc, buffer, BLKSIZE);
        }
        printf("\n%d bytes read from %s\n", totalBytesRead, name);
        printf("Read to byte number %d\n", running->fd[fileDesc]->offset);
        printf("\n%s was found in :\n", name);
        printBlockList(running->fd[fileDesc]->mptr);
        close_file(fileDesc);
    }
}
