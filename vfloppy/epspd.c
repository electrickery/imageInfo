/*
 * epspd.c version 1.2 part of the vfloppy 1.2 package
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
 * epspd - emulates up to four virtual disks for the Epson PX-4 and PX-8
 *         computers using the epsp protocol. 
 * epspd <port> <img1> [<img2> [<img3> [<img4>] ] ]
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "epsp.h"
#include "fdc.h"


int epsp_port;
int drive_fd[4];

int blk_read(int fd, unsigned char *blk, int len)
{
	int sofar=0;
	int syncup=1;
	
	while(sofar<len)
	{
		int got=read(fd,blk,1);
		if(got<=0)
		{
			if(got<0)
			{
				perror("epspd: read");
				exit(1);
			}
			
			if(len==0)
			{
				fprintf(stderr,"epspd: Lost connection.\n");
				exit(1);
			}
		
		}
#if DEBUG>1
		printf(">%02X ",(unsigned int)blk[0]);
#endif
		fflush(stdout);
		sofar+=got;
		blk+=got;
		syncup=0;
	}
#if DEBUG>1
	printf("\n");
#endif
	return sofar;
}

int main(int argc, char *argv[])
{
	struct epsp_msg r;
	struct termios t;
	unsigned char d[32];
	int dv=2;
	
	if(sizeof(r.epsp)!=6)
	{
		fprintf(stderr,"Option botch.\n");
		exit(1);
	}
	
	if(argc<3 || argc> 6)
	{
		fprintf(stderr,"%s port path1..path4.\n",argv[0]);
		exit(1);
	}
	
#if DEBUG>2	
			printf("\nOption count Ok");
#endif		
	
	epsp_port=open(argv[1], O_RDWR);
	if(epsp_port==-1)
	{
		perror(argv[1]);
		exit(1);
	}
#if DEBUG>2	
/*			printf("\nSerial port %s opened", argv[1]); */
#endif		
	
	
	if(tcgetattr(epsp_port, &t)==-1)
	{
		perror("tcgetattr");
		exit(1);
	}
	
	t.c_iflag=t.c_iflag&~(ISTRIP|INLCR|ICRNL|IGNCR|IUCLC|IXON|IXANY|IXOFF);
	t.c_oflag=t.c_oflag&~(OPOST);
	t.c_cflag=t.c_cflag&~(CSIZE|PARENB);
	t.c_cflag|=CS8;
	t.c_lflag=t.c_lflag&~(ISIG|ICANON|ECHO);
	t.c_cc[VTIME]=0;
	t.c_cc[VMIN]=1;
	
	if(tcsetattr(epsp_port, TCSANOW, &t)==-1)
	{
		perror("tcsetattr");
		exit(1);
	}
	
	/*
	 *	Now the disk volumes 
	 */
	 
	drive_fd[1]= -1;
	drive_fd[2]= -1;
	drive_fd[3]= -1;
	
	while(argv[dv])
	{
	
		drive_fd[dv-2]=open(argv[dv],O_RDWR);
		if(drive_fd[dv-2]==-1)
		{
			perror(argv[dv]);
			exit(1);
		}
                printf("\nDisk image %s (fd: %d) mounted ok.",argv[dv],drive_fd[dv-2]);
		dv++;
	}
#if DEBUG>2	
			printf("\nDisk images mounted\n");
#endif		
	
	/*
	 *	Up.. time to go
	 */
	 
	printf("EPSPD: Starting EPSP disk services on %s\n", argv[1]);
	
	 
	while(1)
	{
		int len=blk_read(epsp_port,d,1);
#if DEBUG>1
		printf("\n-> %02X\n ",*d);
#endif
		
		if(d[0]==4)
			continue;
		if(d[0]==5)
		{
#if DEBUG>2	
			printf("\nfdc go ack\n");
#endif		
			fdc_go_ack();
			continue;
		}
					
		if(d[0]!=DID_DE)
		{
#if DEBUG>2	
			printf("\nfirst disk station not selected: %02X\n", d[0]);
#endif		
			fdc_go_nak();
			continue;
		}
		
		/*
		 *	Device selection
		 */

		len=blk_read(epsp_port, d, 3);
		
		
		if(d[0]!=DID_DE && d[0]!=DID_FG)
                {
#if DEBUG>2	
			printf("\nnot DID_DE or DID_FG\n");
#endif		

			continue;
                }
#if DEBUG>2	
		if(d[0]==DID_DE)
			printf("\nFirst disk unit (D:  E:) addressed ");
		if(d[0]==DID_FG)
			printf("\nSecond disk unit (F:  G:) addressed ");
		if(d[1]==SID_MAPLE)
			printf("by a PX-8\n");
		if(d[1]==SID_PINE)
			printf("by a PX-4\n");
#endif		
		
		if(d[2]!=DS_SEL)
		{
#if DEBUG>2	
			printf("\nfdc go nak: no DS_SEL\n");
#endif		
			fdc_go_nak();
			continue;
		}
		
		printf("\nDrive selected\n");
		
		
		/*
		 *	Selected
		 */
		 		 
		fdc_go_ack();
		
		while(1)
		{
			int len=blk_read(epsp_port,d,1);
		
#if DEBUG>1
		printf("\n--> %02X\n ",*d);
#endif
			if(d[0]==EOT)
				break;

			if(d[0]==ENQ)
			{
				fdc_go_ack();
				continue;
			}
					
			if(d[0]!=SOH)
			{
				fdc_go_nak();
				break;
			}
		 
			/*
			 *	Function block
			 */
		 
			len=blk_read(epsp_port, ((unsigned char *)&r.epsp)+1, 6);
			
			r.epsp.head=*d;
		
			if(r.epsp.head!=SOH)
			{
				fdc_go_nak();
				break;
			}
		
			if(r.epsp.fmt!=NUL)
			{
				fdc_go_nak();
				break;
			}
		
			if(r.epsp.did!=DID_DE && r.epsp.did!=DID_FG)
			{
				fdc_go_nak();
				break;
			}
		
			if(checksum((unsigned char *)&r.epsp,7))
			{
				fdc_go_nak();
				break;
			}
		
			/*
			 *	Header probably Ok
			 */
			 
			fdc_go_ack();
#if DEBUG>0	
			printf("\nHeader Ok\n");
#endif		
			/*
			 *	Data block
			 */
	
			blk_read(epsp_port, r.data, r.epsp.siz+4);
			
		
			if(r.data[0]!=STX)
			{
				fdc_go_nak();
				break;
			}
			
			if(checksum(r.data, r.epsp.siz+4))
			{
				fdc_go_nak();
				break;
			}
		
			/*
			 *	All received OK
			 */
		 
			fdc_go_ack();
		
#if DEBUG>1		
			printf("\nGot request ID=%02X DID=%02X SID=%02X, FNC=%02X, SIZ=%02X, D0=%02X, D1=%02X, D2=%02X, ..... \n",
				(int)r.epsp.fmt, (int)r.epsp.did, (int)r.epsp.sid,(int)r.epsp.fnc,
				(int)r.epsp.siz, (int)r.data[1], (int)r.data[2], (int)r.data[3]);
#endif			
			
			blk_read(epsp_port, d, 1);
			
			
			if(d[0]!=EOT)
			{
				fdc_go_nak();
				break;
			}
			
			/*
			 *	Reply now allowed
			 */
		
			switch(r.epsp.fnc)
			{
				case FDC_RESET:
#if DEBUG>2
					printf("Received a FDC_RESET. Execute fdc_cmd_reset\n");
#endif
					fdc_cmd_reset(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_RESET_M:
#if DEBUG>2
					printf("Received a FDC_RESET_M. Execute fdc_cmd_reset\n");
#endif
					fdc_cmd_reset(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_READ:
#if DEBUG>2
					printf("Received a FDC_READ. Execute fdc_cmd_read\n");
#endif
					fdc_cmd_read(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_WRITE:
#if DEBUG>2
					printf("Received a FDC_WRITE. Execute fdc_cmd_write\n");
#endif
					fdc_cmd_write(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_WRITEHST:
#if DEBUG>2
					printf("Received a FDC_WRITEHST. Execute fdc_cmd_writehst\n");
#endif
					fdc_cmd_writehst(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_COPY:
#if DEBUG>2
					printf("Received a FDC_COPY. Execute fdc_cmd_copy\n");
#endif
					fdc_cmd_copy(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_FORMAT:
#if DEBUG>2
					printf("Received a FDC_FORMAT. Execute fdc_cmd_format\n");
#endif
					fdc_cmd_format(&r.epsp, r.data, r.epsp.siz+1);
					break;
				default:
#if DEBUG>2
					printf("Received unknown command. Exiting command loop\n");
#endif
					fdc_go_nak();
					goto end_command_loop;
			}
		}
end_command_loop:				
		
	}
}
