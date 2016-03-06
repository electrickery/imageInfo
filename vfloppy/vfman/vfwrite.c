/*
 * vfwrite.c version 1.0 part of the vfloppy 1.4 package
 *
 * Copyright 2002 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfwrite is placed under the GNU General Public License in July 2002.
 *
 *  This file is part of Vfloppy 1.4.
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
 */

/*
 * vfwrite - writes files to vfloppy image.
 *          used by the vfloppy package.
 * usage: vfwrite <imageName> <fileName>
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


/*
- read arguments
- check and mount image
  (open it read/write, make new function 'mountImageRW'() )
- check file, determine file size (normalize to 2 K blocks) (checkFileNSize)
  if not readable, unmount, exit
- filename to upper case and convert to directory format
- check presence in image, determine size 
  (normalize to 2 K blocks) (checkImgSpace)
- determine required space (free space plus replaced file)
  if too big, unmount, exit
- delete file if it is in the image
- write file to the image
  - locate first empty directory entry (check too many files)
  - write <filename>.$$$ to image
  - create free block list
  - write file to blocks 
  - update directory entry (name and records)
- close, unmount, exit
 */

int main(int argc, char *argv[])
{
	int freeBlocks;
	int oldFileBlks;
	int freeDirEntries;
	int err;

	memset(&data[0], 0x0, BLOCKSIZE);
	memset(&blockList[0], 0x0, MAXBLOCKCOUNT);
	if(argc != 3)
	{
		fprintf(stderr, "%s <imageName> [<fileName>]\n", argv[0]);
		fprintf(stderr, "vfwrite 1.3 (c) F.J. Kraan\n");
		exit(1);
	}
	imageName = argv[1];
	imd = mountImageRW(imageName);

        if(argc == 3)
                fileName = argv[2];

	convertFileName(dirFileName, fileName);
	if ( findFile(imd, dirFileName) != 0 )
	{
		oldFileBlks = countBlocks(&blockList[0]);
		fileDirEntries = oldFileBlks / DIRBLOCKSIZE;
		if ( oldFileBlks % DIRBLOCKSIZE != 0 )
			fileDirEntries++;
	} else {
		oldFileBlks = 0;
		fileDirEntries = 0;
	}
	freeBlocks = DISKBLOCKCOUNT - cntUsedImgBlocks(imd, dirFileName) + oldFileBlks;
	freeDirEntries = DIRENTRYCOUNT - fileDirEntries;
		
	fileSize = checkFileNSize(fileName);
	fileBlks = (fileSize / BLOCKSIZE) + 1;
	if (fileSize % BLOCKSIZE == 0)
		fileBlks--;
		if (debug>1)
			printf("File size appears %d blocks\n", fileBlks);

	if (fileDirEntries == 0)	/* file doesn't exist in image */
		fileDirEntries = fileBlks / DIRBLOCKSIZE + 1;
	if (fileBlks > freeBlocks || fileDirEntries > freeDirEntries)
	{
		printf("Not enough space in image: %d blocks free, %d needed or\n", freeBlocks, fileBlks);
		printf(" in image directory: %d free, %d needed\n", freeDirEntries, fileDirEntries);
		return 0;
	} 
	if (debug>0)
	{
		printf("Enough space in image: %d blocks free, %d needed and \n", freeBlocks, fileBlks);
		printf(" in image directory: %d free, %d needed\n", freeDirEntries, fileDirEntries);
	}
	if (oldFileBlks != 0)
	{
		eraFile(imd, dirFileName);
	}
	getFreeBlockList(imd);
	/* blanking enough of the not used block numbers */
        memset(&blockList[fileBlks], 0x0, DIRBLOCKSIZE);
        writeDir(imd, dirFileName, fileName);
        err = writeBlocks(fileName);
        return 0;
}
