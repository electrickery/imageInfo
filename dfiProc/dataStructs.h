/* 
 * File:   dataStructs.h
 * Author: fjkraan
 *
 * Created on April 9, 2012, 9:58 PM
 */

#ifndef DATASTRUCTS_H
#define	DATASTRUCTS_H

#define MAXSECTORSIZE 1024

typedef struct  {
    int sectorSize;
    unsigned char sectorData[1024];
    int idam;
    int sector;
    int head;
    int track;
    int number;
    int idam_crc1;
    int idam_crc2;
    int dam;
    int data_crc1;
    int data_crc2;
    int encoding;
} sector_t;

#define MAXTRACKSECTORS 30

typedef struct {
    int hardwareTrack;
    int hardwareSide;
    int sectorOrder[30];
    sector_t sector[30];
    int indexMark;
} track_t;

#define JV1_IMG 0
#define JV3_IMG 1
#define DMK_IMG 2
#define IMD_IMG 3
#define D88_IMG 4

typedef struct {
    int skipTracks;
    int out_level;
    int out_file_level;
    int uencoding;
    int kind;
    char *imageFile;
    int imageType;
    FILE *fileHandle;
} userParameters_t ;



#endif	/* DATASTRUCTS_H */

