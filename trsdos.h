/* 
 * File:   trsdos.h
 * Author: fjkraan
 *
 * Created on December 15, 2012, 9:10 PM
 */

#ifndef TRSDOS_H
#define	TRSDOS_H

//    from: http://www.classiccmp.org/cpmarchives/trs80/mirrors/kjsl/www.kjsl.com/trs80/mod1ldos.html
//DIR+0     This byte contains the attributes of the file.
//
//    Bit 7 - 0 = primary DIRREC  1 = secondard DIRREC
//        6 - 0 = user file  1 = system file
//        5 - Reserved
//        4 - 0 = record unused  1 = record in use
//        3 - 0 = file is visible  1 = file is invisible
//      2-0 - File protection level.
//            000 = Full access to the file
//            001 - All but change attributes
//            010 - Rename, write, read, execute
//            011 - unused
//            100 - Write, read, execute
//            101 - Read, execute
//            110 - Execute only
//            111 - No access to the file
//
//DIR+1     Various flags and month of modification
//
//DIR+2     Remaining date of modification flags
//
//DIR+3     End of file offset
//
//DIR+4     Logical Record Length
//
//DIR+5-12  File name, padded with blanks
//
//DIR+13-15 File extension, padded with blanks
//
//DIR+16-17 Update password hash code
//
//DIR+18-19 Access password hash code
//
//DIR+20-21 Ending record number.  Based on full sectors.
//
//DIR+22-23 First file extent field.  The extent is laid out as:
//
//     EXT+0 - One byte containing the cylinder the extent starts on.
//     EXT+1 - Bits 0-4 contain the number of contiguous granules.
//     EXT+1 - Bits 7-5 contain the starting granule on the cylinder.
//
//DIR+24-25 Second file extent field.
//
//DIR+26-27 Third file extent field.
//
//DIR+28-29 Fourth file extent field.
//
//DIR+30    If 0FFh then there is no link to an secondary directory
//          record.  If 0FEh, then there is a link to a FXDE.
//
//DIR+31    If a secondary entry is in use, this contains the 
//          directory entry code (DEC) of the entry.  This DEC
//          is a pointer into the Hash Index Table.  See the
//          information below about the HIT for more information.    
    
    
// from:  TRS-80 HACKER'S HANDBOOK FOR NEWDOS/80 BY KEVIN O'HARE
   
//Byte 0  =  10H  =  File type & Access level.
//Byte 1  =  00H  =  Flags - Bit 7 = ASE :Bit 6 = ASC :Bit 5 = update
//Byte 2  =  00H  =  Storage - not used in NEWDOS/80
//Byte 3  =  B7H  =  EOF offset in the final sector
//                =  Last byte used in final sector
//Byte 4  =  00H  =  Length of each record (0-255, 0=256)
//Bytes 5  to  12  =  ASCII of File name filled with blanks from right
//Bytes 13 to  15  =  ASCII of extension to file name filled as above
//Bytes 16 and 17  =  Update password hash code - if unused = 9642H
//Bytes 18 and 19  =  Access password hash code - if unused = 9642H
//Bytes 20 and 21  =  No. of sectors (from 1) occupied by file (LSB,MSB)
//    If byte 3 =0, 3 byte value (bytes 21,20,3)= No bytes used by file
//    if byte 3 >0 the value is No bytes used +256, =EOF in RBA format
//Byte 22 =  1DH  =  Lump number in which  the file starts (from 0)
//Byte 23 =  20H  =  Bits 0,1,2,3,4 = No. of grans used in this extend
//   Bits 5,6,7   =  1st granule in the lump the file occupies (0 to 7)
//
//          The next 3 pairs of bytes have the same format & function as 22 & 23
//if the extends are used, otherwise all FF's.
//
//Bytes 24 and 25 =  2nd section of contiguous sectors occupied by file
//Bytes 26 and 27 =  3rd section of contiguous sectors occupied by file
//Bytes 28 and 29 =  4th section of contiguous sectors occupied by file
//Byte 30 =  FFH  =  Bit 0 is the flag for the existence of an FXDE
//                   Thus FFH if no FXDE, FEH if one exists.
//Byte 31 =  FFH  =  DEC. Bits 0-4 = Sector number (0-27) of entry in
//   the directory sectors (less 2 used by GAT & HIT) of tthe next FXDE.
//   Bits 5-7 = The FXDE entry number in the sector (= * 32 = byte No.)

// Based on TRSDOS 2.3 Decoded and Other Mysteries [a] (1982)(IJG Inc.):
    
#define DIR_ENTRY_ACCESS_CONTROL 0
#define DIR_ENTRY_OVERFLOW       1
#define DIR_ENTRY_EOF_BYTE_OFFSET 2
#define DIR_ENTRY_RECORD_LENGTH  4
#define DIR_ENTRIES_PER_SECTOR   8
#define DIR_ENTRY_NAME_START     5
#define DIR_ENTRY_NAME_SIZE      8
#define DIR_ENTRY_EXT_START     13
#define DIR_ENTRY_EXT_SIZE       3
#define DIR_ENTRY_INTERVAL    0x20
#define DIR_RESERVED_ENTRY_MAX   2
#define DIR_GAT_SECTOR           0
#define DIR_HIT_SECTOR           1
#define DIR_FIRST_ENTRY_SECTOR   2
#define DIR_SECTORS_TRSDOS23    10
    

typedef struct {
    unsigned char track;            // Lump number in which  the file starts (from 0)
    unsigned char numberOfGranules; // Bits 0,1,2,3,4 = No. of grans used in this extend
} gap_t;                            // Bits 5,6,7     =  1st granule in the lump the file occupies (0 to 7)

typedef struct {
    unsigned char accessControl;  // File type & Access level. bit 6 set = system file
    unsigned char overflow;       // Flags - Bit 7 = ASE :Bit 6 = ASC :Bit 5 = update
    unsigned char reserved;
    unsigned char EOFbyteOffset;  // EOF offset in the final sector
    unsigned char recordLength;   // Length of each record (0-255, 0=256)
    char fileName[DIR_ENTRY_NAME_SIZE];
    char fileExtension[DIR_ENTRY_EXT_SIZE];
    unsigned char updatePasswordMSB; // Update password hash code - if unused = 9642H
    unsigned char updatePasswordLSB; //
    unsigned char accessPasswordMSB; // Access password hash code - if unused = 9642H
    unsigned char accessPasswordLSB; //
    unsigned char EOFsectorLSB;      // No. of sectors (from 1) occupied by file (LSB,MSB)
    unsigned char EOFsectorMSB;      //
    gap_t gap1;
    gap_t gap2;
    gap_t gap3;
    gap_t gap4;
    unsigned char FXDEflag;
    unsigned char FXDElocation;
} directoryEntry_t;

//    Bit 7 - 0 = primary DIRREC  1 = secondard DIRREC
//        6 - 0 = user file  1 = system file
//        5 - Reserved
//        4 - 0 = record unused  1 = record in use
//        3 - 0 = file is visible  1 = file is invisible
//      2-0 - File protection level.
//            000 = Full access to the file
//            001 - All but change attributes
//            010 - Rename, write, read, execute
//            011 - unused
//            100 - Write, read, execute
//            101 - Read, execute
//            110 - Execute only
//            111 - No access to the file

#define DIR_SECONDARY_DIRREC  0x80
#define DIR_SYSTEM_FILE       0x40
#define DIR_RESERVED          0x20
#define DIR_RECORD_IN_USE     0x10
#define DIR_FILE_INVISIBLE    0x08
#define DIR_FILE_ACCESS_MASK  0x07

//#define DIR_AC_FULL_ACCESS    0x00 // KCNRWX  (KILL)
//#define DIR_AC_NO_CHANGE_ATTR 0x01 // -CNRWX
//#define DIR_AC_RENAME_RWX     0x02 // --NRWX  (RENAME)
//#define DIR_AC_UNUSED         0x03
//#define DIR_AC_RWX            0x04 // ---RWX  (WRITE)
//#define DIR_AC_RX             0x05 // ---R-X  (READ)
//#define DIR_AC_X              0x06 // -----X  (EXECUTE)
//#define DIR_AC_NO_ACCESS      0x07 // ------

const char *daAccess[8] = { "KCNRWX", "-CNRWX", "--NRWX", "unused", "---RWX", "---R-X",  "-----X", "------" };
#define DIR_FXDE_SECTOR_MASK  0x1F
#define DIR_FXDE_ENTRY_MASK   0xE0




#endif	/* TRSDOS_H */

