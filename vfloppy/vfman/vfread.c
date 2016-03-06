/*
 * vfread.c version 1.1 part of the vfloppy 1.3 package
 *
 * Copyright 2002 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfread is placed under the GNU General Public License in July 2002.
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
 * vfread - reads directory and files from disk images
 *          used by the vfloppy package.
 * usage: vfread <imageName> [<fileName>]
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "vf.h"

#define PERMS 0664

/*
 - read arguments
 - check and mount image
 - filename to uppercase and convert to directory format
 - parse image directory and return block list
   abort on file not found.
 - create file and dump the blocks
 - close, unmount, exit
 */


int main(int argc, char *argv[])
{
	int freeBlocks;

	entryNo = -1;
	lastRecCnt = -1;
	memset(&data[0], 0x0, BLOCKSIZE);
	memset(&blockList[0], 0x0, MAXBLOCKCOUNT);
	if(argc<2 || argc> 3)
	{
		fprintf(stderr, "%s <imageName> [<fileName>]\n", argv[0]);
		exit(1);
	}
	imageName = argv[1];
	imd = mountImage(imageName);

        if(argc == 3)
		fileName = argv[2];
	else
	{
		if (debug>1)
			printf("To directory mode\n");

		showDir(imd);
		freeBlocks = DISKSIZE - totalUsedBlockCnt;
		printf("%d kByte free (%d blocks, %d records)\n", 
			freeBlocks * 2, 
			freeBlocks,
			freeBlocks * RECSPERBLOCK);
		return 0;
	}
	convertFileName(dirFileName, fileName);

	if (findFile(imd, dirFileName) > 0)
		createFile(fileName); 

	return 0;
}
