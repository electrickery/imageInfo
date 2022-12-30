/* 
 * File:   imageInfo.h
 * Author: fjkraan
 *
 * Created on November 20, 2012, 7:06 PM
 */

#ifndef IMAGEINFO_H
#define IMAGEINFO_H
     
#define MAX_SECTOR_SIZE 1024
#define TRSDOS_SECTOR_SIZE 256
#define TRSDOS_SS_SD_SPT 10
#define TRSDOS_SS_DD_SPT 18

#define JV1_SSSD35_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 35
#define JV1_SSSD40_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 40
#define JV1_SSSD80_SIZE TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE * 80
#define JV1_SSSD80_SECTORS TRSDOS_SS_SD_SPT * 80


void optionParse(int argc, char** argv, userParameters_t *tuserParameters);
void usage(userParameters_t *tuserParameters);

int charArrCheck(char *data1, char *data2, int size);

int checkImage(SectorDescriptor_t *imageIndex, userParameters_t *tuserParameters);
int checkBootSignature(unsigned char *data, unsigned short bootSectorSignature_trsdos23[]);
int getSector(unsigned char *data, SectorDescriptor_t *sectorParams, 
        userParameters_t *tuserParameters);

int jv1Check(userParameters_t *tuserParameters, SectorDescriptor_t *jv1ImageIndex);

int jv3Check(userParameters_t *tuserParameters, SectorDescriptor_t *sectorDescriptor);
unsigned char jv3TranslateSizes(unsigned char jv3HeaderSizes);
void jv3Flags(int logLevel, unsigned char flags);

int dmkCheck(userParameters_t *tuserParameters, SectorDescriptor_t *imageIndex);
int dmkImageParser(SectorDescriptor_t *imageIndex, DMKImageHeader *dmkImageHeader, userParameters_t *tuserParameters);
void dmkGetBootSectorSignature(unsigned char *data, int *dmkSectorIndex, unsigned char *bootSectorSignature);
int dmkGetCRCErrorScore(unsigned char *singledSectorData, int *dmkSectorIndex, int doubleDensity);
void dmkIndexSectorData(unsigned char *data, int *dmkDD256, SectorDescriptor_t *sectorDescriptor);
unsigned short dmkFindDAM(unsigned char *data, int size, int doubleDensity, int startOffset);
unsigned short findOneOfThem(unsigned char *data, int dataSize, int startOffset, unsigned char *validValues, int validValuesSize);
int checkCRC(unsigned char *data, int size, unsigned short crc, int doubleDensity);
unsigned short calc_crc(unsigned short crc, unsigned char byte);

//int readFromImage(unsigned char *data, long location, int size, userParameters_t *tuserParameters);

void showDir(SectorDescriptor_t *sectorParams, SectorDescriptor_t *imageIndex, userParameters_t *userParameters);
void showGAT(int logLevel, unsigned char *data, int size);
void showHIT(int logLevel, unsigned char *data, int size, int sectorsPerTrackSide);
void showDirSector(int logLevel, unsigned char *data, int size);
unsigned char calcHash(char *fileName, int size);

#endif // IMAGEINFO_H
