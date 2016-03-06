/*
 * vf.c version 1.0 part of the vfloppy 1.3 package
 *
 * Copyright 2002 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vf is placed under the GNU General Public License in July 2002.
 *
 *  This file is part of Vfloppy 1.3.
 *
 *  Vfloppy is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Vfloppy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Vfloppy; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * vf - used by vfread and vfwrite
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/errno.h>
#include "vf.h"

#define PERMS 0664

char *fileName;
char dirFileName[12];
char *imageName;
int fnd;
int imd;
unsigned char blockList[MAXBLOCKCOUNT];
unsigned char data[BLOCKSIZE];
int lastRecCnt;
int entryNo;
int totalUsedBlockCnt;
int errno;
int fileSize;
int fileBlks;
int fileDirEntries;
int debug = 3;

/*
 - read arguments
 - check and mount image
 - filename to uppercase and convert to directory format
 - parse image directory and return block list
   abort on file not found.
 - create file and dump the blocks
 - close, unmount, exit
 */

void showDirEntry(unsigned char *dent)
{
	/*
	 * convert data from the directory entry *dent
	 * and represent it as a sortof directory list.
	 * 'local' is for the current directory entry 
	 * only. The global values should reflect the
	 * total size.
	 */
        char filename[13];
	int localEntryNo, recCnt, blockCnt, localLastRecs, i;
	char attr[3];
	attr[0] = ' ';
	attr[1] = ' ';
	attr[2] = 0;

        if (dent[DIRUSERNO] != FMTPATT)
	{
                memcpy(&filename[0],&dent[DIRNAMEBASE],8);
                filename[8]='.';
                memcpy(&filename[9],&dent[DIREXTBASE],3);
                filename[12]=0;
		if ((dent[DIREXTBASE] > 0x7f) != 0)
		{
			attr[0] = 'R';
			filename[9] = 0x7f & filename[9];
		}
		if (dent[DIREXTBASE + 1] > 0x7f)
		{
			attr[1] = 'S';
			filename[10] = 0x7f & filename[10];
		}
		localEntryNo = ((16 * dent[DIRENTRYNOFIELD]) + dent[DIREXTCNTHIGH]) / (EXM + 1);
		recCnt = ((dent[DIRENTRYNOFIELD] / 2) & EXM) * 128 + dent[DIRRECS];
		blockCnt = recCnt / RECSPERBLOCK;
		localLastRecs = recCnt % RECSPERBLOCK;
		if (recCnt != 0)
		{
			if (localLastRecs == 0)
			{
				localLastRecs = 16;
			} else {
				blockCnt++;
			}
		}
                printf("%2d:%s %2s ", dent[DIRUSERNO], filename, attr);
		printf("%6d bytes ", recCnt * RECSIZE);

		if (debug>1)
			printf("(%3d blks, %2d recs in last blk) ", blockCnt, localLastRecs);

                if (dent[DIRENTRYNOFIELD] != 0)
                        printf("dir. entry %d", localEntryNo);
                if (dent[DIRS2] != 0)
                        printf("  ** Corrupt Directory Entry **");
                printf("\n");

		for (i = DIRBLOCKBASE; i <= (DIRBLOCKBASE + DIRBLOCKSIZE); i++)
		{
			if (dent[i] != 0)
				totalUsedBlockCnt++;
		}
        } 
}

int blockRead(int fd, unsigned char *data, unsigned long blockStart, int len)
{
        int err;
        if(lseek(fd, blockStart, 0)==-1)
        {
                perror("vfread: lseek");
                return -1;
        }
        err=read(fd,data,len);
        return err;
}

int blockWrite(int fd, unsigned char *data, unsigned long block, int len)
{
        int err;
        if(lseek(fd, block, 0)==-1)
        {
                perror("vfwrite: lseek");
                return -1;
        }
        err = write(fd,data,len);
        return err;
}

void showDir(int id)
{
	unsigned long dirEntry;
	int i;
	int err;

	/* - loop 64 entries
	 * - start at 0x00080, increment with 0x00020
	 */

	for (i = 0; i < DIRENTRYCOUNT; i++)
	{
		dirEntry = (DIRBASE + i * DIRINCR);
		err = blockRead(id, data, dirEntry, DIRINCR);
        	showDirEntry(data); 
	}
}

int mountImage(char *imName)
{
	if((imd = open(imName,O_RDONLY,0)) == -1)
	{
		fprintf(stderr," failed to open %s\n",imName);
		exit(1);
	}

	if (debug>1)
		printf("Disk image %s (fd: %d) mounted ok (RO).\n",imName,imd);

	return imd;
}

int mountImageRW(char *imName)
{
	if((imd = open(imName,O_RDWR,0)) == -1)
	{
		fprintf(stderr," failed to open %s\n",imName);
		exit(1);
	}
	if (debug>1)
		printf("Disk image %s (fd: %d) mounted ok (RW).\n",imName,imd);

	return imd;
}

void convertFileName(unsigned char *dFName, unsigned char *flName) 
{
	/* conversie fileName dirFileName
	   - to uppercase
	   - add spaces to basename to 8 chars
	   - remove dot
	   - add spaces to extension to 3 chars
	 */
	int i = 0;
	int j = 0;
	int c;
	for (c = 0; c < 11; c++)
		dFName[c] = ' ';
	dFName[11] = '\0';

	/* walk though the array to fill and check the
	 * input array:
	 * - end of string: 	exit
	 * - a dot:		write the rest at the extension location
	 * Most problems are not checked here, but will hopefully fail
	 * nicely at filename comparison.
	 */
	for (i = 0; i < 12; i++)
	{
		if (flName[i] == '\0')
			break;
		if (flName[i] == '.')
		{
			j = 8;
		} else {
			if (j < 11)
			{
				dFName[j] = toupper(flName[i]);
				j++;
			}	
		}
	} 

	if (debug>2)
		printf("Converted filename: \'%s\'\n", dFName); 

}

int cmpEntry(unsigned char *dent, char *dFlName)
{
	char cmpFileName[12];

	/*
	 * - get entry
	 * - if match: return 1 else 0
	 */

	if (dent[DIRUSERNO] == FMTPATT)
	{

		if (debug>2)
			printf("empty or deleted file entry\n");

		return 0;
	}
	memcpy(&cmpFileName[0], &dent[DIRNAMEBASE], DIRNAMESIZE);
	cmpFileName[DIRNAMESIZE] = '\0';

	cmpFileName[8] = 0x7f & cmpFileName[8];	/* clear R/O bit */
	cmpFileName[9] = 0x7f & cmpFileName[9];	/* clear SYS bit */
	if (strcmp(cmpFileName, dFlName) == 0)
	{

	if (debug>1)
		printf("Found file \'%s\' entryNo %d\n", dFlName, dent[DIRENTRYNOFIELD]);

        	return 1;
	} else {
		return 0;
	}
}

void updateBlockList(unsigned char *dent)
{
	int localEntryNo;
	int recCnt, localLastRecs;

	int i;

	/*
	 * store blocklist in array at 'dir entry no' * 16
	 */

	/* dent[DIRENTRYNOFIELD] bit 0 doesn't count (probably last entry flag) */
	localEntryNo = dent[DIRENTRYNOFIELD] / 2;
	/* selects the part of the BlockList the blocks are copied to */
	memcpy(&blockList[localEntryNo * DIRBLOCKSIZE], &dent[DIRBLOCKBASE], DIRBLOCKSIZE);

	if (debug>2)
	{
		printf("Copied blocks to %d\n", (localEntryNo * DIRBLOCKSIZE)); 
		printf("blk: ");
		for (i = 0; i < DIRBLOCKSIZE; i++)
		{
			printf("%d: %X = %X; ", i + localEntryNo * DIRBLOCKSIZE, dent[i + DIRBLOCKBASE], blockList[i + DIRBLOCKSIZE * localEntryNo]);
			if (blockList[i] == '\0')
				break;
		} 
		printf("\n"); 
	}

	/* 
	 * determine the number of used records in the
	 * last allocated block. Done for all entries,
	 * but the globals contain the right data in the end.
	 */
	recCnt = (dent[DIRENTRYNOFIELD] & EXM) * 128 + dent[DIRRECS];
               localLastRecs = recCnt % RECSPERBLOCK;
               if (localLastRecs == 0)
               {
                       localLastRecs = DIRBLOCKSIZE;
               }

	if (localEntryNo > entryNo)
	{
		entryNo = localEntryNo;
		lastRecCnt = dent[DIRRECS];
		if (debug>2)
			printf("updated entryNo: %d and lastRecCnt: %d\n", entryNo, lastRecCnt);
	}
}

int findFile(int id, unsigned char *dFName)
{
	unsigned long dirEntry;
	int i, entryCnt = 0;
	int err;

	/* - loop 64 entries
	 * - start at 0x00080, increment with 0x00020
	 */

	for (i = 0; i < DIRENTRYCOUNT; i++)
	{
		dirEntry = (DIRBASE + i * DIRINCR);
		err = blockRead(id, data, dirEntry, DIRENTRYCOUNT);
		if (cmpEntry(data, dFName) == 1)
		{
			updateBlockList(data);
        		entryCnt = entryCnt++;
		}

		if (debug>2)
			printf("comparing %s with dir.entry %2d@%lX\n", dFName, i, dirEntry); 

 	}

	if (debug>1)
		printf("Found entries: %d\n", entryCnt);

	return entryCnt;
}

int createFile(unsigned char *fileName)
{
	int i, fd, blockSize;
	long unsigned int blkAddr;

	/*
	 * for each block until block number is 0:
	 * - translate block number to address in image
	 * - get block and write it to the file
	 */

	if (unlink(fileName) != 0)
	{

	if (debug>1)
		printf("attempt to unlink current file: %s....\n", fileName);

		if (errno != ENOENT)
		{
			printf("problem deleting %s. Exiting\n", fileName);
			exit(errno);
		} else {
			if (debug>1)
				printf(" file didn't exist in file system\n");
		}

	}
	fd = open(fileName, O_WRONLY|O_CREAT, PERMS);
	if (fd < 0) 
	{
		printf("Error opening file %s\n", fileName);
		return 1;
	} 
	if (debug>1)
	{
		printf("writing file: %s\n", fileName);
		printf("Blocks: ");
	}
	for (i = 0; i < MAXBLOCKCOUNT; i++)
	{
		if (blockList[i] == '\0')
		{
			break;
		}
		blkAddr = BLOCKBASE + blockList[i] * BLOCKSIZE;

		if (debug>1)
			printf("%X: %4lX ", blockList[i], blkAddr);
	
		if (blockList[i + 1] == '\0')
		{
			blockSize = (lastRecCnt % RECSPERBLOCK) * RECSIZE;
			if (blockSize == '\0' && lastRecCnt != '\0')
				blockSize = RECSPERBLOCK;
			if (debug>1)
				printf("\nLast block records: %d\n", lastRecCnt); 
		} else {
			blockSize = BLOCKSIZE;
		}
		blockRead(imd, data, blkAddr, blockSize); 
		write(fd, data, blockSize); 
	}
	if (debug>1)
		printf("\n");

	close(fd); 
	return 0;
}

int checkFileNSize(char *fn)
{
	struct stat fileInfo;
	int bSize, size = 0; 
	int statRet;
	
	statRet = stat(fn, &fileInfo);

/*	printf("statRet returned %d \n", statRet); */
	if (statRet != 0)
	{
		if (statRet == -1)
		{
			fprintf(stderr,"file %s not found. Exiting...\n", fn);
		} else {
			fprintf(stderr,"error statting %s. Exiting...\n", fn);
		}
		exit(statRet);
	}
	size = fileInfo.st_size;
	if (size == 0)
	{
		printf("file is empty. Exiting...\n");
		exit(0);
	}
	bSize = (size / BLOCKSIZE) + 1;
	if (size % BLOCKSIZE == 0)
		bSize--;
	if (debug>1)
		printf("file is %d bytes, %d blocks in file system\n", size, bSize);
	
	return size;
}

int cntUsedImgDirEntries(int id, char *dFn)
{
	int i, err;
	unsigned long dirEntry;
	int count = 0;

        for (i = 0; i < DIRENTRYCOUNT; i++)
        {
		dirEntry = (DIRBASE + i * DIRINCR);
                err = blockRead(id, data, dirEntry, DIRENTRYSIZE);
                if (data[DIRUSERNO] != FMTPATT)
                {
			count++;
		}
	}
	if (debug>1)
		printf("Number of dir entries used: %d\n", count);
	return count;
}

int cntUsedImgBlocks(int id, char *dFn)
{
        int i, j, err, blkLoop;
	unsigned long dirEntry;
	int size = 0;
	for (i = 0; i < DIRENTRYCOUNT; i++)
	{
		dirEntry = (DIRBASE + i * DIRINCR);
		err = blockRead(id, data, dirEntry, DIRENTRYSIZE);
		if (data[DIRUSERNO] != FMTPATT)
		{
			for (j = 0; j < DIRBLOCKSIZE; j++)
			{
				blkLoop = DIRBLOCKBASE + j;
				if (data[blkLoop] != 0)
				{
					size++;
				}

				if (debug>2)
					printf("+%2X=%X:%d ",blkLoop, data[blkLoop], size);

			}
			if (debug>2)
				printf("\n");

		}
	}
	if (debug>2)
		printf("\n");

	return size;
}

int countBlocks(char *blkList)
{
        int i = 0;
	int blockCnt = 0;

        while (blkList[i] != 0)
        {
		i++;
		blockCnt++;
	}
	if (debug>1)
		printf("%d blocks counted for file in image\n", blockCnt);
	return blockCnt;
}

void eraFile(int id, char *dFName)
{
        unsigned long dirEntry;
        int i = 0;
        int err;
        char FmtChar[1];
        FmtChar[0] = FMTPATT;

        /* - loop 64 entries
         * - start at 0x00080, increment with 0x00020
         */

        for (i = 0; i < DIRENTRYCOUNT; i++)
        {
                dirEntry = (DIRBASE + i * DIRINCR);
                err = blockRead(id, data, dirEntry, DIRENTRYCOUNT);
		dFName[8] = 0x7f & dFName[8];	/* clear R/O bit */
		dFName[9] = 0x7f & dFName[9];	/* clear SYS bit */
                if (cmpEntry(data, dFName) == 1)
                {

			if (debug>2)
                	        printf("erasing entry %d of \'%s\'\n", i, dFName);

			blockWrite(id, FmtChar, dirEntry, 1); 
                }
        }
}

void getFreeBlockList(int id)
{
	/* - make a list of all blocks
	 * - parse all used directory entries, zeroing all used blocks
	 * - summarize all remaining bloks in blockList[]
	 */

	int i, j, err;
	unsigned char tmpBlk;
	unsigned char allBlockList[DISKSIZE];
	unsigned long dirEntry;

	memset(&blockList[0], 0x0, MAXBLOCKCOUNT);

	allBlockList[0] = 0;  /* block zero always used by directory */
	for (i = 0; i < DISKSIZE; i++)
	{
		allBlockList[i] = i;
	}
	for (i = 0; i < DIRENTRYCOUNT; i++)
        {
                dirEntry = (DIRBASE + i * DIRINCR);
                err = blockRead(id, data, dirEntry, DIRENTRYCOUNT);
		if (data[DIRUSERNO] != FMTPATT)
		{
			for (j = 0; j < DIRBLOCKSIZE; j++)
			{
				tmpBlk = data[DIRBLOCKBASE + j];
				if (tmpBlk != 0)
					allBlockList[tmpBlk] = 0;
			}
		}
	}
	j = 0;
	for (i = 0; i < DISKSIZE; i++)
	{

		if (debug>2)
			printf("%2X:%2X ", i, allBlockList[i]);

		if (allBlockList[i] != 0)
		{
			blockList[j] = allBlockList[i];
			j++;
		}
	}
	if (debug>2)
		printf("\n");
}

int writeDirEntry(int id, char *dirEnt)
{
	/* 
	 * - loop through dir entries
	 * - when empty:
	 *   - write new entry
	 */
        unsigned long dirEntry;
        int i;
        int err;

        for (i = 0; i < DIRENTRYCOUNT; i++)
        {
                dirEntry = (DIRBASE + i * DIRINCR);
                err = blockRead(id, data, dirEntry, DIRENTRYSIZE);
		if (data[DIRUSERNO] == FMTPATT)
		{
			blockWrite(id, &dirEnt[0], dirEntry, DIRENTRYSIZE);
			return i;
		}
	} 
	return 0;
}

void writeDir(int id, char *dFn, char *fn)
{
	/*
	 * - loop through dir entry sized parts (16 * BLOCKSIZE)
	 *   - fill in file details in temp array
	 *   - write array to image
	 */
	int dirEntryCnt;
	int i;
	int lastEntry = 1;
	int fileRecCnt;
	int dirEntNo;
	unsigned char tmpDirEntry[32];


	dirEntryCnt = (fileSize / (DIRBLOCKSIZE * BLOCKSIZE)) + 1;
	if (fileSize % (DIRBLOCKSIZE * BLOCKSIZE) == 0)
		dirEntryCnt--;
	fileRecCnt = (fileSize / RECSIZE) + 1;
	if (fileSize % RECSIZE == 0)
		fileRecCnt--;

	for (i = 0; i < dirEntryCnt; i++)
	{
		if (i + 1 == dirEntryCnt)
			lastEntry = 0;
		memset(&tmpDirEntry[DIRUSERNO], 0x0, DIRENTRYSIZE);
		tmpDirEntry[0] = 0;
		memcpy(&tmpDirEntry[DIRNAMEBASE], &dirFileName[0], DIRNAMESIZE);
		tmpDirEntry[DIRENTRYNOFIELD] = (i * 2) + lastEntry;
		if (lastEntry == 0)
		{
			tmpDirEntry[DIRRECS] = fileRecCnt % RECSIZE;
		} else {
			tmpDirEntry[DIRRECS] = MAXRECCOUNT;
		}
		memcpy(&tmpDirEntry[DIRBLOCKBASE], &blockList[i * DIRBLOCKSIZE], DIRBLOCKSIZE);
		dirEntNo = writeDirEntry(id, &tmpDirEntry[0]);
 		if (debug>2)
			printf("wrote to dir.entry: %d\n", dirEntNo);
	}
}

int writeBlocks(char *fileName)
{
	int fd, i, blockSize, err;
	unsigned long blkAddr;

        fd = open(fileName, O_RDONLY);
        if (fd < 0)
        {
                printf("Error opening file %s\n", fileName);
                return 1;
        }
	if (debug>1)
	{
        	printf("reading file: %s\n", fileName);
      	  	printf("Blocks: ");
	}

        for (i = 0; i < 256; i++)
        {
                if (blockList[i] == '\0')
                        break;
                blkAddr = BLOCKBASE + blockList[i] * BLOCKSIZE;
		if (debug>2)
	                printf("%X: %4lX ", blockList[i], blkAddr);

                if (blockList[i + 1] == '\0')
                {
                        blockSize = fileSize % BLOCKSIZE;
			if (debug>2)
                        	printf("Last block size: %d\n", blockSize);
			memset(&data[0], 0x0, BLOCKSIZE);
                } else {
                        blockSize = BLOCKSIZE;
                }
		err = read(fd, data, blockSize);
		if (err == -1)
		{
			printf("error while reading %s. Exiting...\n", fileName);
			exit;
		}
                blockWrite(imd, data, blkAddr, blockSize);
        }
	return 0;
}
