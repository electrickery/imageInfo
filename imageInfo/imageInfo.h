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

typedef struct {
    char *imageFile;
    FILE *fileHandle;
    int imageType;
    const char *errorMessage;
} userParameters_t ;

typedef struct {
    unsigned char *data[TRSDOS_SECTOR_SIZE];
    unsigned char track;
    unsigned char sector;
    unsigned char side;
} sectorData_t;


void optionParse(int argc, char** argv, userParameters_t *tuserParameters);
void usage(userParameters_t *tuserParameters);

int openImage(userParameters_t* tuserParameters);
void closeImage(userParameters_t* tuserParameters);

int charArrCheck(char *data1, char *data2, int size);
int getImageSize(userParameters_t* tuserParameters);

int jv1Check(userParameters_t *tuserParameters);
int jv1DirTrack(userParameters_t *tuserParameters);
int getJV1Sector(unsigned char *data, char track, char sector, char side, userParameters_t *tuserParameters);

int jv3Check(userParameters_t *tuserParameters, JV3SectorHeader_t *jv3Headers1, long *jv3SectorIndices);
int getJV3Sector(unsigned char *data, char track, char sector, char side, userParameters_t *tuserParameters, JV3SectorHeader_t *jv3Headers1, long *jv3SectorIndices);
void jv3Flags(unsigned char flags);
void initJV3Structs(long *jv3SectorIndices, JV3SectorHeader_t *jv3Headers1);

int dmkCheck(userParameters_t *tuserParameters, DMK_SectorDescriptor_t *dmkImageIndex);
int getDMKSector(unsigned char *data, char track, char sector, char side, userParameters_t *tuserParameters);
void getRawDMKSector(unsigned char *data, int size, unsigned long location, userParameters_t *tuserParameters);
void getBootSectorSignature(unsigned char *data, int *dmkSectorIndex, unsigned char *bootSectorSignature);
void logDMKData(unsigned char *data, int *dmkDD256, DMK_SectorDescriptor_t *dmkSectorDescriptor, int doubleDensity);

int checkCRC(unsigned char *data, int size, unsigned short crc, int doubleDensity);

void unDoubler(unsigned char *doubledData, unsigned char *singledData, int size, int interval);

unsigned short calc_crc(unsigned short crc, unsigned char byte);