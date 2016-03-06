/*
 * vformat.c version 1.0 part of the vfloppy 1.2 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends.
 *
 * This version delivered in 2002 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfread is placed under the GNU General Public License in July 2002.
 *
 *  This file is part of Vfloppy 1.2.
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
 * vformat - create a file of image size filled with 0xe5's
 * 
 * vformat <imageName>
 */
#include <stdio.h>
#include <fcntl.h>

int blockCount = 0x8b;
int blockSize = 0x800;

main(int argc,char **argv)
{
	int f;
	int i;
	unsigned char buff[blockSize];

	if (argc<2) 
	{
		fprintf(stderr,"Usage: %s <filename>\n",argv[0]);
		exit(0);
	}
	
	memset(buff, 0xe5, blockSize);
	f = open(argv[1], O_WRONLY|O_CREAT, 0666);
	if (f < 0) exit(0);
	for (i = 0; i < blockCount; i++)
		write(f, buff, blockSize);
	write(f, buff, 0x80);
	close(f);
	return 0;
}
