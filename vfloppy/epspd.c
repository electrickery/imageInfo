/*
 * epspd.c version 1.3 part of the vfloppy 1.4 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends.
 *
 * This version delivered in 2002 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfread is placed under the GNU General Public License in July 2002.
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
 *
 */

/*
 * epspd - emulates up to four virtual disks for the Epson PX-4 and PX-8
 *         computers using the epsp protocol. 
 * epspd -s <port> [-0 <img0>] [-1 <img1>] [-2 <img2>] [-3 <img3>] [-d <debuglevel>]
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
int debug=0;
int unit0=0;
int unit1=0;

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
		if (debug >= 2)
		{
			printf(">%02X ",(unsigned int)blk[0]);
		}
		fflush(stdout);
		sofar+=got;
		blk+=got;
		syncup=0;
	}
	if (debug >= 2)
	{
		printf("\n");
	}
	return sofar;
}

int main(int argc, char *argv[])
{
	struct epsp_msg r;
	struct termios t;
	unsigned char d[32];
	int dv=2;
	int c;
        char *d_image0 = "\0";
        char *d_image1 = "\0";
        char *d_image2 = "\0";
        char *d_image3 = "\0";
        char *s_device = "\0";
	char ch;
	char *optstr = "d:0:1:2:3:s:";

	while( -1 != (ch=getopt(argc,argv,optstr))) {
		switch(ch) {
			case '0':           
				d_image0 = optarg;
/*				printf("option: -0 : %s\n", d_image0); */
				break;
			case '1':           
				d_image1 = optarg;
/*				printf("option: -1 : %s\n", d_image1); */
				break;
			case '2':           
				d_image2 = optarg;
/*				printf("option: -2 : %s\n", d_image2); */
				break;
			case '3':           
				d_image3 = optarg;
/*				printf("option: -3 : %s\n", d_image3); */
				break;
			case 's':           
				s_device = optarg;
/*				printf("option: -s : %s\n", s_device); */
				break;
			case 'd':
				debug = atoi(optarg);
/*				printf("option: -d : %d\n", debug); */
				break;
			case '?':
				printf("unrecognized option: %c\n",optopt);
				break;
			default:
				printf("error?  condition unaccounted for?\n");
				break;

		}
	}

	if (debug >= 1)
	{
		printf("\nEPSPD version 1.4 (2003-04-07)");
		printf("\nDebug level: %d", debug);
	}
	if ( s_device[0] != '\0' )
	{
		printf("\nSerial port %s specified", s_device); 
		epsp_port=open(s_device, O_RDWR); 
		if(epsp_port==-1)
		{
			printf("\nError opening serial port %s. Exiting...\n", s_device);
			perror(s_device); 
			exit(2);
		} else {
			printf("\nSerial port \'%s\' (fd: %d) opened succesfully.", s_device, epsp_port); 
		}
	} else {
		printf("\nNo serial port specified. Exiting...\n");
		exit(1);
	}
	
	if(tcgetattr(epsp_port, &t)==-1)
	{
		perror("tcgetattr");
		printf("\nSome error with the serial port\n");
		exit(2);
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
		exit(2);
	}
	
	/*
	 *	Now the disk volumes 
	 */
	 

	if ( d_image0[0] != '\0' )
	{
		drive_fd[0] = open(d_image0, O_RDWR);
		if(drive_fd[0]==-1)
		{
			printf("\nSome error with opening %s. Exiting...\n", d_image0);
			perror(d_image0);
			exit(2);
		}
		printf("\nDisk 0 image \'%s\' (fd: %d) mounted ok.", d_image0, drive_fd[0]);
		unit0 = 1;
	} else {
		if (debug >= 1)
		{
			printf("\nNo image for disk 0 specified");
		}
	}

	if ( d_image1[0] != '\0' )
	{
		drive_fd[1] = open(d_image1, O_RDWR);
		if(drive_fd[1]==-1)
		{
			printf("\nSome error with opening %s. Exiting...\n", d_image1);
			perror(d_image1);
			exit(2);
		}
		printf("\nDisk 1 image \'%s\' (fd: %d) mounted ok.", d_image1, drive_fd[1]);
		unit0 = 1;
	} else {
		if (debug >= 1)
		{
			printf("\nNo image for disk 1 specified");
		}
	} 

	if ( d_image2[0] != '\0' )
	{
		drive_fd[2] = open(d_image2, O_RDWR);
		if(drive_fd[2]==-1)
		{
			printf("\nSome error with opening %s. Exiting...\n", d_image2);
			perror(d_image2);
			exit(2);
		}
		printf("\nDisk 2 image \'%s\' (fd: %d) mounted ok.", d_image2, drive_fd[2]);
		unit1 = 1;
	} else {
		if (debug >= 1)
		{
			printf("\nNo image for disk 2 specified");
		}
	} 

	if ( d_image3[0] != '\0' )
	{
		drive_fd[3] = open(d_image3, O_RDWR);
		if(drive_fd[3]==-1)
		{
			printf("\nSome error with opening %s. Exiting...\n", d_image3);
			perror(d_image3);
			exit(2);
		}
		printf("\nDisk 3 image \'%s\' (fd: %d) mounted ok.", d_image3, drive_fd[3]);
		unit1 = 1;
	} else {
		if (debug >= 1)
		{
			printf("\nNo image for disk 3 specified");
		}
	} 

	if ( unit0 == 0 && unit1 == 0)
	{
		printf("\nNo image at all specified. Exiting...\n");
		exit(1);
	}

if (debug >= 2)	
			printf("\nDisk images mounted\n");
	
	/*
	 *	Up.. time to go
	 */
	 
	printf("\nEPSPD: Starting EPSP disk services on %s", s_device);
	printf("\n");
	
	while(1)
	{
		int len=blk_read(epsp_port,d,1);
		if (debug >= 2)
		{
			printf("\n-> %02X ",*d);
		}
		if(d[0]==EOT)   /* EOT?  ignore */
			continue;
		if(d[0]==ENQ)   /* ENQ?  send an ACK */ 
		{
			if (debug >= 2)	
			{
				printf("\nfdc go ack");
			}
			fdc_go_ack();
			continue;
		}
					
		if(d[0]!=DID_DE)
		{
			if (debug >= 2)	
			{
				printf("\nfirst disk station not selected: %02X", d[0]);
			}
			fdc_go_nak();
			continue;
		}
		
		/*
		 *	Device selection
		 */

		len=blk_read(epsp_port, d, 3);
		
		
		if(d[0]!=DID_DE && d[0]!=DID_FG)
                {
			if (debug >= 2)	
			{
				printf("\nnot DID_DE or DID_FG");
			}
			continue;
                }
		if (debug >= 2)	
		{
			if(d[0]==DID_DE)
				printf("\nFirst disk unit (D:  E:) addressed ");
			if(d[0]==DID_FG)
				printf("\nSecond disk unit (F:  G:) addressed ");
			if(d[1]==SID_MAPLE)
				printf("by a PX-8");
			if(d[1]==SID_PINE)
				printf("by a PX-4");
		}
		
		if(d[2]!=DS_SEL)
		{
			if (debug >= 2)	
			{
				printf("\nfdc go nak: no DS_SEL");
			}
			fdc_go_nak();
			continue;
		}
		
		/* only reply if there is an image mounted in either unit drives */
		if (d[0]==DID_DE && unit0 == 0)
		{
			if (debug >= 2)
			{
				printf("\nIgnoring requests for not defined unit 0");
			}
			continue;
		}
		if (d[0]==DID_FG && unit1 == 0)
		{
			if (debug >= 2)
			{
				printf("\nIgnoring requests for not defined unit 1");
			}
			continue;
		}

		/*
		 *	Selected
		 */

		if (debug >= 2)
		{
			printf("\nDrive selected");
		}
		fdc_go_ack();
		
		while(1)
		{
			int len=blk_read(epsp_port,d,1);
		
		if (debug >= 2)
		{
			printf("\n--> %02X\n ",*d);
		}
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
			if (debug >= 2)	
			{
				printf("\nHeader Ok");
			}
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
		
			if (debug >= 2)	
			{	
				printf("\nGot request ID=%02X DID=%02X SID=%02X, FNC=%02X, SIZ=%02X, D0=%02X, D1=%02X, D2=%02X, .....",
					(int)r.epsp.fmt, (int)r.epsp.did, (int)r.epsp.sid,(int)r.epsp.fnc,
					(int)r.epsp.siz, (int)r.data[1], (int)r.data[2], (int)r.data[3]);
			}
			
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
				case FDC_RESET_P:
					if (debug >= 1)
					{
						printf("\nReceived a FDC_RESET_P. Execute fdc_cmd_reset");
					}
					fdc_cmd_reset(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_RESET_M:
					if (debug >= 1)
					{
						printf("\nReceived a FDC_RESET_M. Execute fdc_cmd_reset");
					}
					fdc_cmd_reset(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_READ:
					if (debug >= 2)
					{
						printf("\nReceived a FDC_READ. Execute fdc_cmd_read");
					}
					fdc_cmd_read(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_WRITE:
					if (debug >= 2)
					{
						printf("\nReceived a FDC_WRITE. Execute fdc_cmd_write");
					}
					fdc_cmd_write(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_WRITEHST:
					if (debug >= 2)
					{
						printf("\nReceived a FDC_WRITEHST. Execute fdc_cmd_writehst");
					}
					fdc_cmd_writehst(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_COPY:
					if (debug >= 2)
					{
						printf("\nReceived a FDC_COPY. Execute fdc_cmd_copy");
					}
					fdc_cmd_copy(&r.epsp, r.data, r.epsp.siz+1);
					break;
				case FDC_FORMAT:
					if (debug >= 2)
					{
						printf("\nReceived a FDC_FORMAT. Execute fdc_cmd_format");
					}
					fdc_cmd_format(&r.epsp, r.data, r.epsp.siz+1);
					break;
				default:
					if (debug >= 2)
					{
						printf("\nReceived unknown command. Exiting command loop");
					}
					fdc_go_nak();
					goto end_command_loop;
			}
		}
end_command_loop:				
		
	}
}
