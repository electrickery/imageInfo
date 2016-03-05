/* 
 * File:   imageInfo.h
 * Author: fjkraan
 *
 * Created on November 20, 2012, 7:06 PM
 */

    
#include "jv3.h"
#include "dmk.h"
     
#define TRSDOS_SECTOR_SIZE 256
#define TRSDOS_SS_SD_SPT 10
#define TRSDOS_SS_DD_SPT 18

#define JV1_SSSD35_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 35
#define JV1_SSSD40_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 40
#define JV1_SSSD80_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 80

#define MINIMUM_TRACKS  34
#define MAXIMUM_TRACKS  86
#define MAXIMUM_SECTORS 28
#define MAXIMUM_SIDES   2

typedef struct {
    char *imageFile;
    FILE *fileHandle;
    int imageType;
    const char *errorMessage;
} userParameters_t ;

//typedef struct {
//    unsigned char *data[TRSDOS_SECTOR_SIZE];
//    unsigned char track;
//    unsigned char sector;
//    unsigned char side;
//} sectorData_t;


void optionParse(int argc, char** argv, userParameters_t *tuserParameters);
void usage(userParameters_t *tuserParameters);

int openImage(userParameters_t* tuserParameters);
void closeImage(userParameters_t* tuserParameters);

int charArrCheck(char *data1, char *data2, int size);
int getImageSize(userParameters_t* tuserParameters);

int checkBootSignature(unsigned char *data, unsigned short bootSectorSignature_trsdos23[]);

int jv1Check(userParameters_t *tuserParameters);
int jv1DirTrack(userParameters_t *tuserParameters);
int jv1GetSector(unsigned char *data, char track, char sector, userParameters_t *tuserParameters);

int jv3Check(userParameters_t *tuserParameters, JV3SectorHeader_t *jv3Headers1, long *jv3SectorIndices);
//int jv3GetSector(unsigned char *data, char track, char sector, char side, userParameters_t *tuserParameters, JV3SectorHeader_t *jv3Headers1, long *jv3SectorIndices);
int jv3GetSector(unsigned char *data, JV3SectorHeader_t *jv3SectorParams, JV3SectorHeader_t *jv3Headers1, long *jv3SectorIndices, userParameters_t *tuserParameters);
void jv3Flags(unsigned char flags);
void jv3InitStructs(long *jv3SectorIndices, JV3SectorHeader_t *jv3Headers1);

int dmkCheck(userParameters_t *tuserParameters, DMK_SectorDescriptor_t *dmkImageIndex);
int dmkTrackParser(DMK_SectorDescriptor_t *dmkImageIndex, DMKImageHeader *dmkImageHeader, userParameters_t *tuserParameters);
int dmkCheckGeometricSanity(DMK_SectorDescriptor_t *dmkImageIndex);
int dmkGetSector(unsigned char *data, DMK_SectorDescriptor_t *tSectorParams, DMK_SectorDescriptor_t *dmkImageIndex, userParameters_t *tuserParameters);
void logBinaryBlock(int logLevel, unsigned char *data, int size);
void dmkFindSectorLocation(DMK_SectorDescriptor_t *tSectorParams, DMK_SectorDescriptor_t *dmkImageIndex);
void dmkGetBootSectorSignature(unsigned char *data, int *dmkSectorIndex, unsigned char *bootSectorSignature);
int dmkGetCRCErrorScore(unsigned char *singledSectorData, int *dmkSectorIndex, int doubleDensity);
void dmkIndexSectorData(unsigned char *data, int *dmkDD256, DMK_SectorDescriptor_t *dmkSectorDescriptor);
unsigned short dmkFindDAM(unsigned char *data, int size, int doubleDensity, int startOffset);
unsigned short findOneOfThem(unsigned char *data, int dataSize, int startOffset, unsigned char *validValues, int validValuesSize);

int checkCRC(unsigned char *data, int size, unsigned short crc, int doubleDensity);

void dmkUnDoubler(unsigned char *doubledData, unsigned char *singledData, int size, int interval);

unsigned short calc_crc(unsigned short crc, unsigned char byte);

//int readFromImage(unsigned char *data, long location, int size, userParameters_t *tuserParameters);
int readFromImage(void *data, long location, int size, userParameters_t *tuserParameters);