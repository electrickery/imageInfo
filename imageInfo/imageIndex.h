/* 
 * File:   imageIndex.h
 * Author: fjkraan
 *
 * Created on March 26, 2014, 9:01 PM
 */

#ifndef IMAGEINDEX_H
#define	IMAGEINDEX_H



#define MINIMUM_TRACKS  34
#define MAXIMUM_TRACKS  86
#define MAX_SECTORS_PER_TRACK 28
#define MAXIMUM_SIDES   2
#define II_SECTORMAX 64*80

#define SD_SIZE_128  0x00
#define SD_SIZE_256  0x01
#define SD_SIZE_512  0x02
#define SD_SIZE_1024 0x03

// masks for SectorDescriptor_t->flags
#define SD_SECTOR_SIZE_MASK  0x03 // bits 0 and 1
#define SD_DATA_DOUBLER_MASK 0x04 // bit 2
#define SD_DOUBLE_DENSITY    0x08 // bit 3


typedef struct {
  unsigned char track;
  unsigned char sector;
  unsigned char side;
  unsigned char flags;
  long sectorLocation;
} SectorDescriptor_t ;

typedef struct {
    unsigned char trackCount;
    unsigned char sideCount;
    unsigned char track0SingleDensity;
    unsigned char track0SectorCount;
    unsigned char track0SectorSize;
    unsigned char track1SingleDensity;
    unsigned char track1SectorCount;
    unsigned char track1SectorSize;
} ImageProperties_t;


void cleanImageIndex(SectorDescriptor_t *imageIndex);
int checkGeometricSanity(SectorDescriptor_t *imageIndex);
void getDiskProperties(ImageProperties_t *diskProperties, SectorDescriptor_t *imageIndex);
int getSectorLocation(SectorDescriptor_t *sectorParams, SectorDescriptor_t *imageIndex);
int getSectorCountTrackSide(SectorDescriptor_t *tSectorParams, SectorDescriptor_t *imageIndex);

#endif	/* IMAGEINDEX_H */

