/*
 * cmd.c version 1.3 part of the vfloppy 1.4 package
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
 * cmd - functions for epspd.c. Implements execution of the commands.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "epsp.h"
#include "fdc.h"

extern int epsp_port;
extern int drive_fd[4];
extern int debug;
extern int unit0;
extern int unit1;

void show_dir_entry(unsigned char *block)
{
	int offset;
	char filename[13];
	int size,part;
	for (offset=0; offset<128; offset+=32)
	{
		printf("\n%d: ",offset/32);
		if (block[offset]==0xe5) 
			printf("No File");
		else
		{
			memcpy(&filename[0],&block[offset+1],8);
			filename[8]='.';
			memcpy(&filename[9],&block[offset+9],3);
			filename[12]=0;
			size=block[offset+15];
			part=block[offset+12];

			printf("%s %3d records ",filename,size);
			printf("part %d of %d",part, (size/(16*8))+1);
			if (block[offset+13]!=0 || block[offset+14]!=0)
				printf("  ** Corrupt Directory Entry **");
		}
	}
}

void fdc_go_ack()
{
	char dp=ACK;
	write(epsp_port, &dp, 1);	
        if (debug >= 2)
	{
		printf("\nSent ACK");
	}
}

void fdc_go_nak()
{
	char dp=NAK;
	write(epsp_port, &dp, 1);	
        if (debug >= 2)
	{
		printf("\nSent NAK");
	}
}


unsigned char checksum(unsigned char *bp, int len)
{
	unsigned char sum=0;
	while(len--)
		sum+=*bp++;
	return 256-sum;
}

int log_write(int fd, unsigned char *data, int len)
{
	int ct;
        if (debug >= 2)
	{
		printf("\nc>");
		for(ct=0;ct<len;ct++)
			printf("%02X ",data[ct]);
	}
	return write(fd,data,len);
        if (debug >= 2)
	{
        	printf("\n");
	}
}

static void send_epsp(struct epsp_msg *r)
{
	unsigned char d[32];

	/*
	 *	Write the header
	 */

	r->epsp.head=0x01;
        if (debug >= 2)
	{
		printf("\nSend EPSP %02X %02X %02X %02X %02X %02X",
			r->epsp.fmt,r->epsp.did,r->epsp.sid, r->epsp.fnc,
			r->epsp.siz, r->data[0]);
	}
	if(log_write(epsp_port, (unsigned char *)r, 6)==-1)
		perror("epspd: write error");
	d[0]=checksum((unsigned char *)&r->epsp, 6);
	if(log_write(epsp_port, d,1)==-1)
		perror("epspd: write error");
	
	/*
	 *	Wait for ACK/NAK
	 */
	 
	blk_read(epsp_port, d, 1);
	
	if(d[0]==NAK)
	{
        	if (debug >= 1)
		{
			printf("\nNAKkered\n");
		}
		return;
	}
	
	if(d[0]==0x05)
	{
        	if (debug >= 2)
		{
			printf("\nReceived ENQ");
		}
		fdc_go_ack();
		return;
	}
	
	if(d[0]!=ACK)
	{
        	if (debug >= 1)
		{
			printf("\nReceived no ACK but %02X", d[0]);
		}
		return;
	}
	
	
	/*
	 *	Command reply is ACK'd
	 */
	
	
	r->data[0]=0x02;
	r->data[r->epsp.siz+2]=0x03;
	r->data[r->epsp.siz+3]=checksum(r->data,r->epsp.siz+3);
	log_write(epsp_port,r->data,r->epsp.siz+4);
	
	/*
	 *	Data sent
	 */
	

	/*
	 *	Wait for ACK/NAK
	 */
	 
	blk_read(epsp_port, d, 1);
	
	if(d[0]==NAK)
	{
        	if (debug >= 2)
		{
			printf("\nNAKkered");
		}
		return;
	}
	
	if(d[0]==ENQ)
	{

        	if (debug >= 2)
		{
			printf("\nENQ");
		}
		fdc_go_ack();
		return;
	}
	
	if(d[0]!=ACK)
	{
        	if (debug >= 2)
		{
			printf("\nReceived no ACK but %02X", d[0]);
		}
		return;
	}
	
	/*
	 *	Data ACK'd send EOT
	 */
	 
	
	d[0]=EOT;
	log_write(epsp_port,d,1);
	if (debug >= 2)
	{
		printf("\n");
	}
}

#define ADJUST 4

static int BLOCK_MAP(int a,int b)
{
	if (debug >= 2)
	{
		printf("\nBLOCK_MAP: Track %d Sector %d == 0x%lX\n",a,b,(((a-0)*64L)+b)*128L);
	}
	return((((a-ADJUST)*64L)+b)*128L);
}

static int block_read(int fd, unsigned char *data, unsigned long block, int len)
{
	int err;
	if(lseek(fd, block, 0)==-1)
	{
		perror("epspd: lseek");
		return -1;
	}
	err=read(fd,data,len);
	if (debug >= 2)
	{
		printf("DISK: %d READ: pos=0x%lX len=%d\n", fd, block, len);
	}
	if (block>(0x2000*(4-ADJUST)) && block < (0x2000*((4-ADJUST)+2)))
	{
		if (debug >= 2)
		{
			show_dir_entry(data);
		}
		return err;
	}
return err;
}

static int block_write(int fd, unsigned char *data, unsigned long block, int len)
{
	int err;
	if(lseek(fd, block, 0)==-1)
	{
		perror("epspd: lseek");
		return -1;
	}
	err=log_write(fd,data,len);
	if (debug >= 2)
		printf("DISK: %d WRITE: pos=0x%lX len=%d\n", fd, block, len);

	if (block>(0x2000*(4-ADJUST)) && block < (0x2000*((4-ADJUST)+2)))
	{
		if (debug >= 2)
		{
			show_dir_entry(data);
		}
		return err;
	}
return err;
}

static int drive_of(unsigned char a, unsigned char b)
{
        /* a = a - 0x31 */
	a-=0x31;
	if(a>1)
	{
		fprintf(stderr, "EPSP: bad drive (%d) using 0\n", a);
		return 0;
	}
        /* a = a * 2, left shift one position unary operator */
	a<<=1;
	/* b = b -1 */
	b--;
	if(b>1)
	{
		fprintf(stderr,"EPSP: bad disk (%d) using 0\n", b);
		return 1;
	}
	/* a = a + b */
	a+=b;
	return a;
	
}

/* struct epsp *s       = the message from the PX, used to create return message header
   unsigned char *bytes = contains track and sector data. data[256]
   int len              = ignored, supposed to be always 128
 */
void fdc_cmd_reset(struct epsp *s, unsigned char *bytes,int len)
{
	struct epsp_msg reply;
	reply.epsp.fmt=FMT_FROM_FDD;
	reply.epsp.sid=s->did;
	reply.epsp.did=s->sid;
	reply.epsp.fnc=s->fnc;
	reply.epsp.siz=0;
	reply.data[1]=FDC_ERR_OK;
	send_epsp(&reply);
}

void fdc_cmd_read(struct epsp *s, unsigned char *bytes, int len)
{
	struct epsp_msg reply;
	reply.epsp.fmt=FMT_FROM_FDD;
	reply.epsp.sid=s->did;
	reply.epsp.did=s->sid;
	reply.epsp.siz=128;
	reply.epsp.fnc=s->fnc;
	if(block_read(drive_fd[drive_of(s->did,bytes[1])], 
                      reply.data+1,
		      BLOCK_MAP(bytes[2],bytes[3]),
		      128)!=128)
	{
		reply.data[129]=FDC_ERR_READ;
		send_epsp(&reply);
	}
	else
	{
		reply.data[129]=FDC_ERR_OK;
		send_epsp(&reply);
	}
}

void fdc_cmd_write(struct epsp *s, unsigned char *bytes, int len)
{
	struct epsp_msg reply;
	reply.epsp.fmt=FMT_FROM_FDD;
	reply.epsp.sid=s->did;
	reply.epsp.did=s->sid;
	reply.epsp.siz=0;
	reply.epsp.fnc=s->fnc;
	if(block_write(drive_fd[drive_of(s->did, bytes[1])], 
		       bytes+5,
                       BLOCK_MAP(bytes[2],bytes[3]),
		       128)!=128)
	{
		reply.data[1]=FDC_ERR_WRITE;
		perror("write");
		send_epsp(&reply);
	}
	else
	{
		reply.data[1]=FDC_ERR_OK;
		send_epsp(&reply);
	}
}

void fdc_cmd_writehst(struct epsp *s, unsigned char *bytes, int len)
{
	struct epsp_msg reply;
	reply.epsp.fmt=FMT_FROM_FDD;
	reply.epsp.sid=s->did;
	reply.epsp.did=s->sid;
	reply.epsp.siz=0;
	reply.epsp.fnc=s->fnc;
	reply.data[1]=FDC_ERR_OK;
	send_epsp(&reply);
}

void fdc_cmd_copy(struct epsp *s, unsigned char *bytes, int len)
{
	/*
	 *	We don't support copy.
	 */
	struct epsp_msg reply;
	reply.epsp.fmt=FMT_FROM_FDD;
	reply.epsp.sid=s->did;
	reply.epsp.did=s->sid;
	reply.epsp.siz=0;
	reply.epsp.fnc=s->fnc;
	reply.data[1]=FDC_ERR_SELECT;
	send_epsp(&reply);
}

void fdc_cmd_format(struct epsp *s, unsigned char *bytes, int len)
{
	/*
	 *	Easy 8)
	 */
	struct epsp_msg reply;
	int ct=0;
	while(ct<20)
	{
		reply.epsp.fmt=FMT_FROM_FDD;
		reply.epsp.sid=s->did;
		reply.epsp.did=s->sid;
		reply.epsp.siz=2;
		reply.data[1]=0;
		reply.data[2]=ct;
		reply.data[3]=FDC_ERR_OK;
		reply.epsp.fnc=s->fnc;
		send_epsp(&reply);
		ct++;
		sleep(1);
		/* We ought to play 'kerchunk' samples. */
	}
	reply.epsp.fmt=FMT_FROM_FDD;
	reply.epsp.sid=s->did;
	reply.epsp.did=s->sid;
	reply.epsp.siz=2;
	reply.data[1]=255;
	reply.data[2]=255;
	reply.data[3]=FDC_ERR_OK;
	reply.epsp.fnc=s->fnc;
	send_epsp(&reply);
}	
