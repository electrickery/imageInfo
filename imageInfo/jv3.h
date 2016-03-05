/* 
 * File:   jv3.h
 * Author: fjkraan
 *
 * Created on November 28, 2012, 5:16 PM
 */

// Info indirectly from Phil Ereaut:
//
//JV3_DAM value   Single density       Double density
//0x00            0xFB (Normal)        0xFB (Normal)
//0x20            0xFA (User-defined)  0xF8 (Deleted)
//0x40            0xF9 (User-defined)  Invalid; unused
//0x60            0xF8 (Deleted)       Invalid; unused

#define SD_0xFB_DAM 0x00 // (Normal)
#define SD_0xFA_DAM 0x20 // (User-defined)
#define SD_0xF9_DAM 0x40 // (User-defined) (used for Model I directory tracks)
#define SD_0xF8_DAM 0x60 // (Deleted)
#define DD_0xFB_DAM 0x00 // (Normal)
#define DD_0xF8_DAM 0x20 // (Deleted)
#define DD_0x40_DAM 0x40 // Invalid; unused
#define DD_0x60_DAM 0x60 // Invalid; unused

typedef struct {
  unsigned char track;
  unsigned char sector;
  unsigned char flags;
} JV3SectorHeader_t;

#define JV3_SECTORS 2901

typedef struct {
  JV3SectorHeader_t headers1[JV3_SECTORS]; // 8703, 0x21FF
  unsigned char writeprot;     //    0xFF is R/W, 0x00 is Write protected
  unsigned char data1[];       //    
//  JV3SectorHeader headers2[JV3_SECTORS]; // 8703
//  unsigned char padding;       //    0xFF
//  unsigned char data2[];       //
} JV3_t;

typedef struct {
   JV3SectorHeader_t *headers1[JV3_SECTORS]; 
   long *sectorLocations[JV3_SECTORS];
} JV3ImageIndex_t;

// values in flag field:
#define JV3_DENSITY     0x80  /* 1=dden, 0=sden */
#define JV3_DAM         0x60  /* data address mark code; see below */
#define JV3_SIDE        0x10  /* 0=side 0, 1=side 1 */
#define JV3_ERROR       0x08  /* 0=ok, 1=CRC error */
#define JV3_NONIBM      0x04  /* 0=normal, 1=short */
#define JV3_SIZE        0x03  /* in used sectors: 0=256,1=128,2=1024,3=512
                                 in free sectors: 0=512,1=1024,2=128,3=256 */

#define JV3_FREE        0xFF  /* in track and sector fields of free sectors */
#define JV3_FREEF       0xFC  /* in flags field, or'd with size code */



