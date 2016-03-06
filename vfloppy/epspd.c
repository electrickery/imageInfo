/*
 * epspd.c version 1.1 part of the vfloppy 1.1 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends. 
 *
 * This version is delivered by Fred Jan Kraan (fjkraan@xs4all.nl) in
 * June 2002.
 *
 * This software is placed under the GPL in June 2002.
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
	unsigned char *dp=blk;
	int want=1;
	
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
		printf("%02X ",(unsigned int)blk[0]);
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

void main(int argc, char *argv[])
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
	
	epsp_port=open(argv[1], O_RDWR);
	if(epsp_port==-1)
	{
		perror(argv[1]);
		exit(1);
	}
	
	
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
	 *	Now the disk volume (just one for now)
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
		dv++;
	}
	
	/*
	 *	Up.. time to go
	 */
	 
	printf("EPSPD: Starting EPSP disk services on %s\n", argv[1]);
	
	 
	while(1)
	{
		int len=blk_read(epsp_port,d,1);

		if(*d==4)
			continue;
		if(*d==5)
		{
			fdc_go_ack();
			continue;
		}
					
		if(*d!=0x31)
		{
			fdc_go_nak();
			continue;
		}
		
		/*
		 *	Device selection
		 */

		len=blk_read(epsp_port, d, 3);
		
		
		if(d[0]!=0x31 && d[0]!=0x32)
                {
			continue;
                }
		
		if(d[2]!=0x05)
		{
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
		printf("\n%02X\n ",*d);
#endif
			if(*d==4)
				break;

			if(*d==5)
			{
				fdc_go_ack();
				continue;
			}
					
			if(*d!=0x01)
			{
				fdc_go_nak();
				break;
			}
		 
			/*
			 *	Function block
			 */
		 
			len=blk_read(epsp_port, ((unsigned char *)&r.epsp)+1, 6);
			
			r.epsp.head=*d;
		
			if(r.epsp.head!=0x01)
			{
				fdc_go_nak();
				break;
			}
		
			if(r.epsp.fmt!=0x00)
			{
				fdc_go_nak();
				break;
			}
		
			if(r.epsp.did!=0x31 && r.epsp.did!=0x32)
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
			
		
			if(r.data[0]!=0x02)
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
			printf("\nGot request ID=%02X DID=%02X SID=%02X, FNC=%02X, SIZ=%02X D0=%02X\n",
				(int)r.epsp.fmt, (int)r.epsp.did, (int)r.epsp.sid,(int)r.epsp.fnc,
				(int)r.epsp.siz, (int)r.data[1]);
#endif			
			
			blk_read(epsp_port, d, 1);
			
			
			if(*d!=0x04)
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
					fdc_cmd_reset(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_RESET_M:
					fdc_cmd_reset(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_READ:
					fdc_cmd_read(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_WRITE:
					fdc_cmd_write(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_WRITEHST:
					fdc_cmd_writehst(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_COPY:
					fdc_cmd_copy(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_FORMAT:
					fdc_cmd_format(&r.epsp, r.data, r.epsp.siz+1);
					break;
				default:
					fdc_go_nak();
					goto fuck;
			}
		}
fuck:				
		
	}
}
