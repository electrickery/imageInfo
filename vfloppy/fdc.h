/*
 * fdc.h version 1.2 part of the vfloppy 1.2 package
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
 * fdc.h - EPSP codes for fdc commands
 *
 */
 
#define FDC_RESET	0x0D
#define FDC_RESET_M	0x0E

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


