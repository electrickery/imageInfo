/*
 * fdc.h version 1.1 part of the vfloppy 1.1 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends. 
 *
 * This version is delivered by Fred Jan Kraan (fjkraan@xs4all.nl) in
 * June 2002.
 *
 * This software is placed under the GPL in June 2002.
 */

/*
 *	EPSP codes for fdc's
 */
 
#define FDC_RESET	0x0D
#define FDC_RESET_M	0x0E  /* PX-8 reset */

#define FDC_READ	0x77
#define FDC_WRITE	0x78
#define FDC_WRITEHST	0x79
#define FDC_COPY	0x7A
#define FDC_FORMAT	0x7C

#define FDC_ERR_OK	0x00
#define FDC_ERR_READ	0xFA
#define FDC_ERR_WRITE	0xFB
#define FDC_ERR_SELECT	0xFC
#define FDC_ERR_WRITEP	0xFD


