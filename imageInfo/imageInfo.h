/* 
 * File:   imageInfo.h
 * Author: fjkraan
 *
 * Created on November 20, 2012, 7:06 PM
 */

    
#include "jv3.h"
#include "dmk.h"
     
#define MAX_SECTOR_SIZE 1024
#define TRSDOS_SECTOR_SIZE 256
#define TRSDOS_SS_SD_SPT 10
#define TRSDOS_SS_DD_SPT 18

#define JV1_SSSD35_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 35
#define JV1_SSSD40_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 40
#define JV1_SSSD80_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 80
#define JV1_SSSD80_SECTORS TRSDOS_SS_SD_SPT * 80

#define MINIMUM_TRACKS  34
#define MAXIMUM_TRACKS  86
#define MAXIMUM_SECTORS 28
#define MAXIMUM_SIDES   2

typedef struct {
  unsigned char track;
  unsigned char sector;
  unsigned char side;
  unsigned char flags;
  long sectorLocation;
} SectorDescriptor_t ;

#define II_SECTORMAX 64*80

// masks for SectorDescriptor_t->flags
#define SD_SECTOR_SIZE_MASK  0x03 // bits 0 and 1
#define SD_DATA_DOUBLER_MASK 0x04 // bit 2
#define SD_DOUBLE_DENSITY    0x08 // bit 3

#define SD_SIZE_128  0x00
#define SD_SIZE_256  0x01
#define SD_SIZE_512  0x02
#define SD_SIZE_1024 0x03

typedef struct {
    char *imageFile;
    FILE *fileHandle;
    int imageType;
    const char *errorMessage;
    int showDirectory;
    int forceSingleSided;
} userParameters_t ;

typedef struct {
    unsigned char trackCount;
    unsigned char sideCount;
    unsigned char track0SingleDensity;
    unsigned char track0SectorCount;
    unsigned char track0SectorSize;
    unsigned char track1SingleDensity;
    unsigned char track1SectorCount;
    unsigned char track1SectorSize;
} DiskProperties_t;

void optionParse(int argc, char** argv, userParameters_t *tuserParameters);
void usage(userParameters_t *tuserParameters);

int openImage(userParameters_t* tuserParameters);
void closeImage(userParameters_t* tuserParameters);

int charArrCheck(char *data1, char *data2, int size);
int getImageSize(userParameters_t* tuserParameters);
void getDiskProperties(DiskProperties_t *diskProperties, SectorDescriptor_t *imageIndex);

int checkBootSignature(unsigned char *data, unsigned short bootSectorSignature_trsdos23[]);
int getSectorLocation(SectorDescriptor_t *sectorParams, SectorDescriptor_t *imageIndex);

int jv1Check(userParameters_t *tuserParameters, SectorDescriptor_t *jv1ImageIndex);
int getSector(unsigned char *data, SectorDescriptor_t *sectorParams, 
        SectorDescriptor_t *imageIndex, userParameters_t *tuserParameters);

int jv3Check(userParameters_t *tuserParameters, SectorDescriptor_t *sectorDescriptor);
unsigned char jv3TranslateSizes(unsigned char jv3HeaderSizes);
void jv3Flags(int logLevel, unsigned char flags);

int dmkCheck(userParameters_t *tuserParameters, SectorDescriptor_t *imageIndex);
int dmkTrackParser(SectorDescriptor_t *imageIndex, DMKImageHeader *dmkImageHeader, userParameters_t *tuserParameters);
int dmkCheckGeometricSanity(SectorDescriptor_t *imageIndex);
void logBinaryBlock(int logLevel, unsigned char *data, int size);
void dmkGetBootSectorSignature(unsigned char *data, int *dmkSectorIndex, unsigned char *bootSectorSignature);
int dmkGetCRCErrorScore(unsigned char *singledSectorData, int *dmkSectorIndex, int doubleDensity);
void dmkIndexSectorData(unsigned char *data, int *dmkDD256, SectorDescriptor_t *sectorDescriptor);
unsigned short dmkFindDAM(unsigned char *data, int size, int doubleDensity, int startOffset);
unsigned short findOneOfThem(unsigned char *data, int dataSize, int startOffset, unsigned char *validValues, int validValuesSize);

int checkCRC(unsigned char *data, int size, unsigned short crc, int doubleDensity);

void dmkUnDoubler(unsigned char *doubledData, unsigned char *singledData, int size, int interval);

unsigned short calc_crc(unsigned short crc, unsigned char byte);

//int readFromImage(unsigned char *data, long location, int size, userParameters_t *tuserParameters);
int readFromImage(void *data, long location, int size, userParameters_t *tuserParameters);

void showGAT(int logLevel, unsigned char *data, int size);
void showHIT(int logLevel, unsigned char *data, int size, DiskProperties_t *diskProperties);
void showDIR(int logLevel, unsigned char *data, int size);
unsigned char calcHash(char *fileName, int size);
void cleanImageIndex(SectorDescriptor_t *imageIndex);