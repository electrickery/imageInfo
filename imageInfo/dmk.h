/* 
 * File:   dmk.h
 * Author: fjkraan
 *
 * Created on November 28, 2012, 5:13 PM
 */


// Info indirectly from Phil Ereaut:
//000000:   00       28      00 19       00          00      00      00 00 00 00 00 00 00 00 00
//          Write   Tracks  Track    D/Sided(0)        D/Dens(0)
//         Protect            Length   S/Sided(1)        S/Dens(1)
// 
//trkindex = Track Length * 256
// 
//If trkindex = 6400 Then dmkflag = 1: dens = 2: spg = 6: damindex = 44
//If trkindex = 3264 Then dmkflag = 1: dens = 1: spg = 5: damindex = 24
//
// 
//If DMKflag = 1 it is a DMK otherwise it is a JV3



// The magic values should never appear in images on disk, as it is a flag to the 
// emulator that the drive is a real disk drive, not an image file on disk.
typedef struct {
    unsigned char writeProtect;
    unsigned char tracks;
    unsigned char trackLengthLSB; // to prevent alignment of the short
    unsigned char trackLengthMSB; // it is defined as two chars
    unsigned char options;
    unsigned char byte5; // reserved
    unsigned char byte6; // reserved
    unsigned char byte7; // reserved
    unsigned char byte8; // reserved
    unsigned char byte9; // reserved
    unsigned char byteA; // reserved
    unsigned char byteB; // reserved
    unsigned char byteC; // magic value 0x12 for real disks or 0 for virtual disks
    unsigned char byteD; // magic value 0x34 for real disks or 0 for virtual disks
    unsigned char byteE; // magic value 0x56 for real disks or 0 for virtual disks
    unsigned char byteF; // magic value 0x78 for real disks or 0 for virtual disks
} DMKImageHeader;

#define DMK_SECTOR_POINTERS_IN_HEADER 64
typedef struct {
    unsigned char sectorPointerLSB; // reverse order
    unsigned char sectorPointerMSB; // contains flags at bit 6 & 7
} DMKSectorPointer;

typedef struct {
    DMKSectorPointer revRpointer[DMK_SECTOR_POINTERS_IN_HEADER];
} DMKTrackHeader;

// Disk header flags in byte 4
#define DMK_SINGLE_SIDED_ONLY          0x10 // bit 4
#define DMK_SINGLE_DENSITY_SINGLE_BYTE 0x40 // bit 6
#define DMK_IGNORE_DENSITY             0x80 // bit 7 

// Sector header flags in sectorPointerMSB
#define DMK_SP_MASK_UNDEFINED       0x40 // bit 6
#define DMK_DOUBLE_DENSITY_SECTOR   0x80 // bit 7
#define DMK_SECTOR_POINTER_MSB_MASK 0x3F // bits 0-5

#define DMK_RAW_SECTOR_SIZE 1024 + 1024 + 100

// Source: FD 179X-01 Floppy Disk Formatter/Controller Family

//                 0  1  2  3  4   5   6  7   8   9    10 
//                ID Tr Sd Sc EN IcM  IcL DM  Dat DcM  DcL
int dmkSD256[] = {0, 1, 2, 3, 4,  5,  6, 24, 25, 281, 282};
int dmkDD256[] = {0, 1, 2, 3, 4,  5,  6, 44, 45, 301, 302};

#define DMK_RAW_SECTOR_INDEX_ID  0
#define DMK_RAW_SECTOR_INDEX_TR  1
#define DMK_RAW_SECTOR_INDEX_SD  2
#define DMK_RAW_SECTOR_INDEX_SC  3
#define DMK_RAW_SECTOR_INDEX_EN  4
#define DMK_RAW_SECTOR_INDEX_ICM 5
#define DMK_RAW_SECTOR_INDEX_ICL 6
#define DMK_RAW_SECTOR_INDEX_DM  7
#define DMK_RAW_SECTOR_INDEX_DAT 8
#define DMK_RAW_SECTOR_INDEX_DCM 9
#define DMK_RAW_SECTOR_INDEX_DCL 10

#define DMK_IDAM_CRC_RANGE 5
#define DMK_DAM_CRC_RANGE_OVERHEAD 1


typedef struct {
    unsigned char IDAM; // 0xFE
    unsigned char trackNumber;
    unsigned char sideNumber;
    unsigned char sectorNumber;
    unsigned char ENC;
    unsigned char IDAM_CRC_MSB; // to prevent alignment of the short
    unsigned char IDAM_CRC_LSB; // it is read as two chars
    unsigned char FFs[11];
    unsigned char _00s[6];
    unsigned char DAM; // FB, F9
    unsigned char data[256];
    unsigned char dataCRC_MSB; // to prevent alignment of the short
    unsigned char dataCRC_LSB; // it is read as two chars
} DMK_SD_SECTOR_256;

typedef struct {
    unsigned char IDAM; // 0xFE
    unsigned char trackNumber;
    unsigned char sideNumber;
    unsigned char sectorNumber;
    unsigned char ENC;    
    unsigned char IDAM_CRC_MSB; // to prevent alignment of the short
    unsigned char IDAM_CRC_LSB; // it is read as two chars
    unsigned char _4Es[22];
    unsigned char _00s[12];
    unsigned char _F5s[3];
    unsigned char DAM; // FB
    unsigned char data[256];
    unsigned char dataCRC_MSB; // to prevent alignment of the short
    unsigned char dataCRC_LSB; // it is read as two chars
} DMK_DD_SECTOR_256;

typedef struct {
  unsigned char track;
  unsigned char sector;
  unsigned char side;
  unsigned char flags;
  long sectorLocation;
} DMK_SectorDescriptor_t;

#define DMK_II_SECTORMAX 64*80

#define DMK_SECTOR_SIZE_MASK  0x03 // bits 0 and 1
#define DMK_DATA_DOUBLER_MASK 0x04 // bit 2

#define DMK_II_128  0x00
#define DMK_II_256  0x01
#define DMK_II_512  0x02
#define DMK_II_1024 0x03
