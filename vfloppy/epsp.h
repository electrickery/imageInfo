/*
 * epsp.h version 1.1 part of the vfloppy 1.1 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends. 
 *
 * This version is delivered by Fred Jan Kraan (fjkraan@xs4all.nl) in
 * June 2002.
 *
 * This software is placed under the GPL in June 2002.
 */

/*
 *	Epson serial protocol
 */
 

struct epsp
{
	unsigned char head;
	unsigned char fmt;
	unsigned char did;
	unsigned char sid;
	unsigned char fnc;
	unsigned char siz;
	
};

struct epsp_msg
{
	struct epsp epsp __attribute__((packed));
	unsigned char data[256] __attribute__((packed));
};

#define FMT_FROM_PINE	0x00
#define FMT_FROM_FDD	0x01

#define DID_FD1		0x31
#define DID_FD2		0x32

#define SID_MAPLE	0x22
#define SID_PINE	0x23

