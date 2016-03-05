/* 
 * File:   main.cpp
 * Author: fjkraan
 *
 * Created on November 20, 2012, 7:04 PM
 */

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "imageInfo.h"

#include "logger.h"
#include "crc.h"
#include "imageSignatures.h"

/* ImageInfo -  analyse disk images for typical TRS-80  formats, being JV1, JV3 and DMK
 *              This tool checks the properties (file size) and structure of a file and
 *              decides which type it is. The only part of the image data (sector 
 *              contents) that is checked are the first two bytes of the boot sector.
 * 
 * fjkraan, 2012-12-14
 */

int main(int argc, char** argv) {
    userParameters_t ups;
    userParameters_t *userParameters = &ups;
    userParameters->errorMessage = "";
    userParameters->fileHandle = NULL;
    unsigned char dt[TRSDOS_SECTOR_SIZE];
    unsigned char *data = &dt[0];

    JV3SectorHeader_t jv3H1[JV3_SECTORS]; 
    JV3SectorHeader_t *jv3Headers1 = &jv3H1[0];
    JV3SectorHeader_t jsp;
    JV3SectorHeader_t *jv3SectorParams = &jsp;
    long jv3si[JV3_SECTORS];
    long *jv3SectorIndices = &jv3si[0];
    DMK_SectorDescriptor_t dii[DMK_II_SECTORMAX];
    DMK_SectorDescriptor_t *dmkImageIndex = &dii[0];
    DMK_SectorDescriptor_t sp;
    DMK_SectorDescriptor_t *dmkSectorParams = &sp;

    optionParse(argc, argv, userParameters);
    
    if (!openImage(userParameters)) {
        logger(ERROR, "Error opening file '%s'", userParameters->imageFile);
        exit(1);
    }
    
    if (jv1Check(userParameters)) {
        jv1GetSector(data, 0, 0, userParameters);
        if (checkBootSignature(data, bootSectorSignature_trsdos23)) {
            logger(INFO, "Boot sector looks like TRSDOS 2.x or similar DOS\n");
        }
        exit(0);
    }
    
    jv3InitStructs(jv3SectorIndices, jv3Headers1);
    
    if (jv3Check(userParameters, jv3Headers1, jv3SectorIndices)) {
        jv3SectorParams->track  = 0;
        jv3SectorParams->sector = 0;
        jv3SectorParams->flags  = 0; // relevant bits: side 0, size 256 bytes
        jv3GetSector(data, jv3SectorParams, jv3Headers1, jv3SectorIndices, userParameters);
        if (checkBootSignature(data, bootSectorSignature_trsdos23)) {
            logger(INFO, "Boot sector looks like TRSDOS 2.x or similar DOS\n");
            exit(0);
        }
        if (checkBootSignature(data, bootSectorSignature_trsdos27)) {
            logger(INFO, "Boot sector looks like TRSDOS 2.7\n");
            exit(0);
        }
        exit(0);
    }
    
    if (dmkCheck(userParameters, dmkImageIndex)) {
        dmkSectorParams->track  = 0;
        dmkSectorParams->sector = 0;
        dmkSectorParams->side   = 0;
        dmkGetSector(data, dmkSectorParams, dmkImageIndex, userParameters);
        if (checkBootSignature(data, bootSectorSignature_trsdos23)) {
            logger(INFO, "Boot sector looks like TRSDOS 2.x or similar DOS\n");
            exit(0);
        }
        if (checkBootSignature(data, bootSectorSignature_trsdos27)) {
            logger(INFO, "Boot sector looks like TRSDOS 2.7\n");
            exit(0);
        }
        exit(0);
    }

    logger(WARN, "Image type '%s' not recognised\n", userParameters->imageFile);
    closeImage(userParameters);
    
    exit(0);
}

int checkBootSignature(unsigned char *data, unsigned short bootSectorSignature[]) {
    int i, b;
    int signatureSize = 0;
    unsigned short shortedData;
    
    while (signatureSize < TRSDOS_SECTOR_SIZE && bootSectorSignature[signatureSize] < BOOTSECTORSIGNATURE_TERMINATOR) {
        signatureSize++;
    }
    for (i = 0; i < signatureSize; i++) {
        logger(TRACE, "Check sig - %d: %02X == %02X\n", i, data[i], bootSectorSignature[i]);
        shortedData = data[i];
        if (shortedData == BOOTSECTORSIGNATURE_IGNORE || shortedData == BOOTSECTORSIGNATURE_DIR_TRACK) continue;
        if (data[i] != bootSectorSignature[i]) return 0;
    }
    return 1;
}

#define MAX_LOG_LEVEL 5
void optionParse(int argc, char** argv, userParameters_t *tuserParameters) {
    if (argc <= 1) {
        tuserParameters->errorMessage = "no arguments";
        usage(tuserParameters);
        exit(1);
    }
    const char *optstr = "f:hv:?";
    char ch;
    int level;
    if (argc <= 1) usage(tuserParameters);
    while( -1 != (ch=getopt(argc,argv,optstr))) {
        switch(ch) {
            case 'f':
                tuserParameters->imageFile = optarg;
                break;                
            case 'h':
                usage(tuserParameters);
                exit(1);
                break;
            case 'v':
                level = strtol(optarg, NULL, 0);
                if (level < 0) level = 0;
                if (level > MAX_LOG_LEVEL) level = MAX_LOG_LEVEL;
                setLogLevel(level);
                break;
            case '?':
            default:
                logger(ERROR, "Error: condition unaccounted for\n");
                usage(tuserParameters);
                exit(1);
                break;
        }
    }
    if (strncmp(tuserParameters->imageFile, "", 1) == 0) {
        tuserParameters->errorMessage = "Error: no output image file specified";
        usage(tuserParameters);
        exit(1);
    }   
}

void usage(userParameters_t *tuserParameters) {
    logger(ERROR, "imageInfo version 0.6\n");
    if (strncmp(tuserParameters->errorMessage, "", 1) != 0) {
        logger(ERROR, "%s\n", tuserParameters->errorMessage);
    }
    logger(ERROR, "usage: -f <image file>\n");
    logger(ERROR, "       -h              - help and version\n");
    logger(ERROR, "       -v <log level>  - verbosity (0-5), default 1.\n");
    logger(ERROR, "                         0 - ERROR; 1 - WARN;  2 - INFO; \n");
    logger(ERROR, "                         3 - DEBUG; 4 - TRACE; 5 - BYTES\n");
    logger(ERROR, "       -?              - help and version\n");

}

// The JV1 has no overhead data, all bytes are sector contents. The main limitation is that
// only single sided, single density disks can be represented with 10 256 byte sectors per 
// track. So the resulting files must have one of a set of sizes. Currently 35, 40 and 80 
// track disk sizes are checked for.
// Another aspect is that the sectors are in their proper number order.
int jv1Check(userParameters_t *tuserParameters) {
    logger(INFO, "JV1 check:\n");
    int size = getImageSize(tuserParameters);
    
    if (size == JV1_SSSD35_SIZE) {
        logger(INFO, "Size is correct for a 35 tracks, 10 sectors of 256 bytes JV1 image\n");
    } else if (size == JV1_SSSD40_SIZE) {
        logger(INFO, "Size is correct for a 40 tracks, 10 sectors of 256 bytes JV1 image\n");
    } else if (size == JV1_SSSD80_SIZE) {
        logger(INFO, "Size is correct for a 80 tracks, 10 sectors of 256 bytes JV1 image\n");
    } else {
        logger(INFO,"Size of %d bytes does not match a standard JV1 image\n", size);
        return 0;
    }
    logger(WARN, "Image is a ");
    logger(ERROR, "JV1\n");
    return 1;
}

int jv1GetSector(unsigned char *data, char track, char sector, userParameters_t *tuserParameters) {
    int location = track * TRSDOS_SS_SD_SPT * TRSDOS_SECTOR_SIZE + sector * TRSDOS_SECTOR_SIZE;
    
    readFromImage(data, location, TRSDOS_SECTOR_SIZE, tuserParameters);

    return(1);
}

// The JV3 does not really have a identifying header, but each track is preceeded 
// with a array describing the order and type of the sectors on that track.
// One check is iterating over these arrays for all tracks and count the maximum 
// track, side and sector number is a simple check. If the track and sector 
// count is not inside a reasonable range, the image isn't a valid JV3. 
// Another task of this function is building an sectorIndex, containing the location
// of each sector. Together with the JV3 sector header, copied from the image, 
// this can be used to retrieve the sector from the image.
int jv3Check(userParameters_t *tuserParameters, JV3SectorHeader_t *jv3Headers1, long *jv3SectorIndices) {
    int result;
    unsigned char track, sector, flags;
    int headerBlockSize = JV3_SECTORS * sizeof(JV3SectorHeader_t); 
    long wpFlagLocation = headerBlockSize; // points to the next byte
    int bootSectorIndex = 0;
    int trackHighCount = 0;
    int sectorHighCount = 0;
    int sides = 0;
    int previousTrack = 0;
    int previousOffset = headerBlockSize + 1; // header + writeProtect
    int previousSectorSize = 0;
    int sectorSize = TRSDOS_SECTOR_SIZE;
    unsigned char wpfl[1];
    unsigned char *wpFlag = &wpfl[0];
    
    logger(INFO, "JV3 check:\n");
    
    result = readFromImage(jv3Headers1, 0, headerBlockSize, tuserParameters);
    
    int i;
    for (i = 0; i < JV3_SECTORS; i++) {
        track  = jv3Headers1[i].track;
        sector = jv3Headers1[i].sector;
        flags  = jv3Headers1[i].flags;
        if (track == 255 || sector == 255) { // This indicates no further sectors are in this header.
            continue;
        } else {
            trackHighCount = track > trackHighCount ? track : trackHighCount;
            sectorHighCount = sector > sectorHighCount ? sector : sectorHighCount;
            sides  = ((int)flags & JV3_SIDE) ? 2 : 1;
            if (track == 0 && sector == 0 && ((int)flags & JV3_SIDE) == 0) bootSectorIndex = i; 
        }
        previousTrack = track;
        if (((int)flags & JV3_SIZE) == 0) sectorSize = 256;
        if (((int)flags & JV3_SIZE) == 1) sectorSize = 128;
        if (((int)flags & JV3_SIZE) == 2) sectorSize = 1024;
        if (((int)flags & JV3_SIZE) == 3) sectorSize = 512;
        jv3SectorIndices[i] = previousOffset + previousSectorSize;
        previousOffset = jv3SectorIndices[i];
        previousSectorSize = sectorSize;
        logger(DEBUG, "SectorHeader index: %d", i);
        logger(TRACE, " (location: %05X)", jv3SectorIndices[i]);
        logger(DEBUG, ", track: %d, sector: %d, flags: %d ", track, sector, flags);
        jv3Flags(flags);
    }
    logger(INFO, "Highest track: %d, Sides: %d, ", trackHighCount, sides);
    result = readFromImage(&wpFlag, wpFlagLocation, 1, tuserParameters); // the next byte after headers1
    logger(DEBUG, "(0x%02X) ", wpFlag);
    if (wpFlag[0] == JV3_WPFLAG_OFF) {
        logger(INFO, "Write Enabled\n");
    } else {
        logger(INFO, "Write Protected\n");
    }
        
    if (trackHighCount >= MINIMUM_TRACKS && trackHighCount < MAXIMUM_TRACKS && sectorHighCount < MAXIMUM_SECTORS) {
        logger(WARN, "Image is a ");
        logger(ERROR, "JV3");
        logger(WARN, " image");
        logger(ERROR, "\n");
        return 1; // only sane values found for a normal disk.
    }
    return 0;
}

int jv3GetSector(unsigned char *data, JV3SectorHeader_t *jv3SectorParams, JV3SectorHeader_t *jv3Headers1, long *jv3SectorIndices, userParameters_t *tuserParameters) {
    int i = 0;
    long location = 0;
    int result;
    unsigned char side;
    
    for (i = 0; i < JV3_SECTORS; i++) {
        if (jv3SectorParams->track  == jv3Headers1[i].track && 
            jv3SectorParams->sector == jv3Headers1[i].sector && 
            (jv3SectorParams->flags & JV3_SIDE) == (jv3Headers1[i].flags & JV3_SIDE)) {
            side = (jv3Headers1[i].flags & JV3_SIDE) ? 1 : 0;
            location = jv3SectorIndices[i];
            printf("Idx: %d; Found sector %d, track %d, side %d at %05lX\n", i, jv3Headers1[i].sector, jv3Headers1[i].track, side, location);
        }
    }
    if (location == 0) return 0;
    result = readFromImage(data, location, TRSDOS_SECTOR_SIZE, tuserParameters);
  
    return 1;
}

void jv3InitStructs(long *jv3SectorIndices, JV3SectorHeader_t *jv3Headers1) {
    int i;
    for (i = 0; i < JV3_SECTORS; i++) {
        jv3SectorIndices[i] = 0;
        jv3Headers1->sector = 0xFF;
        jv3Headers1->track  = 0xFF;
    }
}

// The DMK image does have a image header, but it does not contain an identifier.
// Four sizes are defined for the image; 5 1/4" single and double density and 8"
// Single And DOuble Density. If the size bytes contain other information, the 
// image isn't a DMK (but I found one other format in the wild so I added that 
// too). The header specifies also if single density sectors are
// written as such, or each byte is present twice. 
// The track header contains a list of relative addresses (unsigned short, 14 bit), 
// each pointing to the IDAM of a sector. One of the remaining two bits of the 
// address contains the actual density of the sector. In case of double density,
// the doubling flag is to be ignored. For each sector the IDAM and DAM crc are
// checked.
// Each sector address in the image file is stored in an array with the track, 
// side and sector number, for retrieval of specific sectors later (read directory,
// file retrieval). There is also a flag field, containing useful information like
// sector size (128 to 1024 are supported) and if the bytes are written twice, a 
// feature of DMK used for single density.
int dmkCheck(userParameters_t *tuserParameters, DMK_SectorDescriptor_t *dmkImageIndex) {
    int result;
    DMKImageHeader dih;
    DMKImageHeader *dmkImageHeader = &dih;
    int diskSize5;
    int diskDoubleDensity;
    int writeProtect;
    int singleDensitySingleByte;
    int crcCheckErrors;
    
    logger(INFO, "DMK check:\n");
       
    result = readFromImage(dmkImageHeader, 0, sizeof(DMKImageHeader), tuserParameters);
    
    writeProtect = dmkImageHeader->writeProtect;
    singleDensitySingleByte = dmkImageHeader->options & DMK_SINGLE_DENSITY_SINGLE_BYTE;
    
    logger(DEBUG, "Image: tracks: %d, ", dmkImageHeader->tracks);
    unsigned short trackLength = dmkImageHeader->trackLengthLSB + 
        dmkImageHeader->trackLengthMSB * 256;
    if (trackLength == 0x0CC0) { 
        logger(DEBUG, "Single Density 5 1/4\"");
        diskDoubleDensity = 0;
        diskSize5 = 1;
    } else if (trackLength == 0x1900) {
        logger(DEBUG, "Double Density 5 1/4\"");
        diskDoubleDensity = 1;
        diskSize5 = 1;
    } else if (trackLength == 0x1980) { // This size is not in the original spec
        logger(DEBUG, "Double Density 5 1/4\", Single Density sectors only"); 
        diskDoubleDensity = 1;
        diskSize5 = 1;
//        return 0;
    } else if (trackLength == 0x14E0) {
        logger(DEBUG, "Single Density 8\"");
        diskDoubleDensity = 0;
        diskSize5 = 0;
    } else if (trackLength == 0x2940) {
        logger(DEBUG, "Double Density 8\"");
        diskDoubleDensity = 1;
        diskSize5 = 0;
    } else {
        logger(DEBUG, "unknown DMK type, trackLength: 0x%04X.\n", trackLength);
        return 0; // No reason to continue.
    }
    logger(DEBUG, " (0x%04X), ", trackLength);
    logger(DEBUG, "%s, ", (dmkImageHeader->options & DMK_SINGLE_SIDED_ONLY) ? "Single Sided" : "Double Sided" );
    if (writeProtect == 0xFF) {
        logger(WARN, "Write Protected ");
    } else if (writeProtect == 0x00) {
        logger(WARN, "Write Enabled ");
    } else {
        logger(WARN, "unknown write state: 0x%02X (should be Write Protected (0xFF) or Write Enabled (0x00)) ", writeProtect);
    }
    logger(TRACE, ", %s density (obsolete), ", 
            ((dmkImageHeader->options & DMK_IGNORE_DENSITY) == 1) ? "Ignore" : "Respect");
    logger(TRACE, "SD sector bytes are written %s", (singleDensitySingleByte) ? "once" : "twice");
    
    crcCheckErrors = dmkTrackParser(dmkImageIndex, dmkImageHeader, tuserParameters);
    
    if (crcCheckErrors == 0) {
        logger(DEBUG, " All CRCs ok");
    } else {
        logger(DEBUG, " %d CRC errors (IDAM & DAM)", crcCheckErrors);
    }
    logger(DEBUG, ".\n");

    if (!dmkCheckGeometricSanity(dmkImageIndex)) return 0;
    return 1; // valid
}

int dmkTrackParser(DMK_SectorDescriptor_t *dmkImageIndex, 
        DMKImageHeader *dmkImageHeader, userParameters_t *tuserParameters) {
    int i = 0, result, imageIndex = 0;
    int trackCount = 0;
    int crcCheckErrors = 0;
    unsigned char bootSectorSignature[3];
    
    DMKTrackHeader dt1h;
    DMKTrackHeader *dmkTrack1Header = &dt1h;
    unsigned char sd[DMK_RAW_SECTOR_SIZE];
    unsigned char *sectorData = &sd[0];

    int doubleDensity = 0;
    unsigned short sectorPointer;
    unsigned short nextSectorPointer;
    unsigned short rawSectorPointer;
    unsigned char ssd[DMK_RAW_SECTOR_SIZE];
    unsigned char *singledSectorData = &ssd[0];
    int singleDensitySingleByte = dmkImageHeader->options & DMK_SINGLE_DENSITY_SINGLE_BYTE;
    int sectorLevelSingleByte = singleDensitySingleByte;
    unsigned short trackLength = dmkImageHeader->trackLengthLSB + 
    dmkImageHeader->trackLengthMSB * 256;

    for (trackCount = 0; trackCount < dmkImageHeader->tracks; trackCount++) {
        long trackHeaderLocation = sizeof(DMKImageHeader) + trackLength * trackCount;
        logger(TRACE, "---- Track %d: %05X ----\n", trackCount, trackHeaderLocation);
        result = readFromImage(dmkTrack1Header, trackHeaderLocation, sizeof(DMKTrackHeader), tuserParameters);

        for (i = 0; i < DMK_SECTOR_POINTERS_IN_HEADER; i++) {
            sectorPointer = dmkTrack1Header->revRpointer[i].sectorPointerLSB + 
                    (dmkTrack1Header->revRpointer[i].sectorPointerMSB & DMK_SECTOR_POINTER_MSB_MASK) * 256;
            rawSectorPointer = dmkTrack1Header->revRpointer[i].sectorPointerLSB + 
                    dmkTrack1Header->revRpointer[i].sectorPointerMSB * 256;
            doubleDensity = (dmkTrack1Header->revRpointer[i].sectorPointerMSB & DMK_DOUBLE_DENSITY_SECTOR);
            sectorLevelSingleByte = dmkTrack1Header->revRpointer[i+1].sectorPointerMSB & DMK_DOUBLE_DENSITY_SECTOR;
            if (i < DMK_SECTOR_POINTERS_IN_HEADER - 1) {
                nextSectorPointer = dmkTrack1Header->revRpointer[i+1].sectorPointerLSB + 
                    (dmkTrack1Header->revRpointer[i+1].sectorPointerMSB & 0x3F) * 256; 
            }
            if (sectorPointer == 0) {
                logger(TRACE, "highest sector is %d\n", i);
                break;
            }
            logger(TRACE, "Sector pointer(0x%04X + 0x%05X): 0x%04X, Density: %s - ", 
                    rawSectorPointer, trackHeaderLocation, sectorPointer + trackHeaderLocation, (doubleDensity) ? "double" : "single");
            readFromImage(sectorData, sectorPointer + trackHeaderLocation, DMK_RAW_SECTOR_SIZE, tuserParameters);
            if (doubleDensity) { // The double byte issue is only for single density
                logger(TRACE, "Copying single byte DD sector\n");
                dmkUnDoubler(sectorData, singledSectorData, DMK_RAW_SECTOR_SIZE, 1);
            } else {
                if (sectorLevelSingleByte) { // 
                    logger(TRACE, "Copying single byte SD sector\n");
                    dmkUnDoubler(sectorData, singledSectorData, DMK_RAW_SECTOR_SIZE, 1);
                } else {
                    logger(TRACE, "Reducing double byte SD sector\n");
                    dmkUnDoubler(sectorData, singledSectorData, DMK_RAW_SECTOR_SIZE, 2);
                }
            }
            int realSectorSize = 128<<singledSectorData[dmkRawSector[DMK_RAW_SECTOR_INDEX_EN]];
            int damLocation = 
                dmkFindDAM(singledSectorData, DMK_RAW_SECTOR_SIZE, doubleDensity, dmkRawSector[DMK_RAW_SECTOR_INDEX_ICL] + 1);
            dmkRawSector[DMK_RAW_SECTOR_INDEX_DM]  = damLocation;
            dmkRawSector[DMK_RAW_SECTOR_INDEX_DAT] = damLocation + DMK_DAM_DATA;
            dmkRawSector[DMK_RAW_SECTOR_INDEX_DCM] = damLocation + DMK_DAM_DcM_OFFSET + realSectorSize;
            dmkRawSector[DMK_RAW_SECTOR_INDEX_DCL] = damLocation + DMK_DAM_DcL_OFFSET + realSectorSize;
            logger(TRACE, "(Loc DAM: %d, Data: %d, DAMcrc: %d %d) ", 
                    dmkRawSector[DMK_RAW_SECTOR_INDEX_DM], dmkRawSector[DMK_RAW_SECTOR_INDEX_DAT],
                    dmkRawSector[DMK_RAW_SECTOR_INDEX_DCM], dmkRawSector[DMK_RAW_SECTOR_INDEX_DCL]);

            dmkIndexSectorData(singledSectorData, &dmkRawSector[0], &dmkImageIndex[imageIndex]); 
            crcCheckErrors += dmkGetCRCErrorScore(singledSectorData, &dmkRawSector[0], doubleDensity);

            long sectorDataLocation = 
            sectorPointer + trackHeaderLocation + dmkRawSector[DMK_RAW_SECTOR_INDEX_DAT] * (sectorLevelSingleByte ? 1 : 2);
            dmkImageIndex[imageIndex].sectorLocation = sectorDataLocation;
            dmkImageIndex[imageIndex].flags += (sectorLevelSingleByte && !doubleDensity) ? DMK_DATA_DOUBLER_MASK : 0;
            logger(TRACE, "---- Tr.%d, Sd.%d, Sc.%d @ 0x%06X ----\n", 
                    dmkImageIndex[imageIndex].track, dmkImageIndex[imageIndex].side, 
                    dmkImageIndex[imageIndex].sector, dmkImageIndex[imageIndex].sectorLocation);
            imageIndex++;
        }
    }
    return crcCheckErrors;
}

int dmkCheckGeometricSanity(DMK_SectorDescriptor_t *dmkImageIndex) {
    int dii;
    int trackHighCount = 0;
    int sectorHighCount = 0;
    int sideHighCount = 0;
    for (dii = 0; dii < DMK_II_SECTORMAX; dii++) {
        if (dmkImageIndex[dii].sectorLocation == 0) break;
        logger(BYTES, "Index: %d; Tr: %d; Sd: %d; Sc: %d; Location: %05X; Flags %X\n", 
                dii,
                dmkImageIndex[dii].track, 
                dmkImageIndex[dii].side, 
                dmkImageIndex[dii].sector, 
                dmkImageIndex[dii].sectorLocation, 
                dmkImageIndex[dii].flags);
        if (dmkImageIndex[dii].track > trackHighCount) trackHighCount = dmkImageIndex[dii].track;
        if (dmkImageIndex[dii].sector > sectorHighCount) sectorHighCount = dmkImageIndex[dii].sector;
        if (dmkImageIndex[dii].side > sideHighCount) sideHighCount = dmkImageIndex[dii].side;
    }
    logger(TRACE, "Track max: %d, Side max: %d, Sector max: %d\n", trackHighCount, sideHighCount, sectorHighCount);
    
    if (trackHighCount >= MINIMUM_TRACKS && trackHighCount < MAXIMUM_TRACKS && sectorHighCount < MAXIMUM_SECTORS && sideHighCount < MAXIMUM_SIDES) {
        logger(WARN, "Image is a ");
        logger(DEBUG, "%s Sided ", (dmkImageIndex[dii].side > 0) ? "Double" : "Single");
        logger(ERROR, "DMK");
        logger(WARN, " image");
        logger(DEBUG, " with %d tracks and %d sectors (MAX) per track.", trackHighCount + 1, sectorHighCount);
        logger(ERROR, "\n");
        return 1; // only sane values found for a normal disk.
    }
    return 0; // insane values
}

void dmkIndexSectorData(unsigned char *data, int *dmkSectorIndex, DMK_SectorDescriptor_t *dmkImageIndex) {
    unsigned char idam     = data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_ID]];
    unsigned char track    = data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_TR]];
    unsigned char side     = data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_SD]];
    unsigned char sector   = data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_SC]];
    unsigned char size     = data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_EN]];
    unsigned char dam      = data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_DM]];
    
    logger(TRACE, "IDAM: %02X, ", idam);
    logger(TRACE, "Track: %d, Side: %d, Sector: %d, Size: %d, ", track, side, sector, 128<<size);
    logger(TRACE, "DAM: %02X, ", dam);
    
    dmkImageIndex->track  = track;
    dmkImageIndex->side   = side;
    dmkImageIndex->sector = sector;
    dmkImageIndex->sectorLocation = 0; // set in dmkTrackParser()
    dmkImageIndex->flags  = size;      // other flag set in dmkTrackParser(), dmkFindSectorLocation()
}

int dmkGetCRCErrorScore(unsigned char *data, int *dmkSectorIndex, int doubleDensity) {
    int idamCRCok, damCRCok;
    int crcErr = 0;
    int size     = 128<<(data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_EN]]);
    
    unsigned short idamCRC = data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_ICM]] * 256 + 
                             data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_ICL]];
    idamCRCok = checkCRC(&data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_ID]], 
            DMK_IDAM_CRC_RANGE, idamCRC, doubleDensity);
    logger(TRACE, "IDAMcrc: %04X: ", idamCRC);
    logger(TRACE, "%s, ", (idamCRCok) ? "ok" : "NOT ok"); 
    
    unsigned short damCRC  = data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_DCM]] * 256 + 
                             data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_DCL]];
    damCRCok = checkCRC(&data[dmkSectorIndex[DMK_RAW_SECTOR_INDEX_DM]], 
            size + DMK_DAM_CRC_RANGE_OVERHEAD, damCRC, doubleDensity);
    logger(TRACE, "DAMcrc: %04X: ", damCRC);
    logger(TRACE, "%s", (damCRCok) ? "ok" : "NOT ok"); // DMK_DAM_CRC_RANGE_OVERHEAD
    
    crcErr = (idamCRCok ? 0 : 1) + (damCRCok ? 0 : 1);
    if (crcErr) {
        logger(TRACE, " CRC Err: %d ", crcErr);
    }
    logger(TRACE, "\n");  
    return crcErr;
}

int dmkGetSector(unsigned char *data, DMK_SectorDescriptor_t *tSectorParams, DMK_SectorDescriptor_t *dmkImageIndex, userParameters_t *tuserParameters) {
    unsigned char rdt[TRSDOS_SECTOR_SIZE * 2];
    unsigned char *rawData = &rdt[0];
    int i;
    
    dmkFindSectorLocation(tSectorParams, dmkImageIndex);
    logger(TRACE, "Found track %d, sector %d, side %d at location %05X\n", 
            tSectorParams->track, tSectorParams->sector, tSectorParams->side, tSectorParams->sectorLocation);
    
    if (tSectorParams->flags & DMK_DATA_DOUBLER_MASK) {
        readFromImage(rawData, tSectorParams->sectorLocation, TRSDOS_SECTOR_SIZE, tuserParameters);
        logger(DEBUG, "sector copied:\n");
        dmkUnDoubler(rawData, data, TRSDOS_SECTOR_SIZE, 1);
    } else {
        readFromImage(rawData, tSectorParams->sectorLocation, TRSDOS_SECTOR_SIZE * 2, tuserParameters);
        logger(DEBUG, "sector reduced:\n");
        dmkUnDoubler(rawData, data, (TRSDOS_SECTOR_SIZE * 2), 2);
    }
    return 0;
}

void logBinaryBlock(int logLevel, unsigned char *data, int size) {
#define BYTES_PER_LINE 16
    int i, lineCount;
    int halfWay = BYTES_PER_LINE / 2 - 1; // ugly off-by-one
    char textPart[BYTES_PER_LINE + 1];
    textPart[BYTES_PER_LINE] = 0;       // end of string
    int lines = size / BYTES_PER_LINE;
    for (lineCount = 0; lineCount < size; lineCount += BYTES_PER_LINE) {
        logger(logLevel, "%04X: ", lineCount);
        for (i = 0; i < BYTES_PER_LINE; i++) {
            logger(logLevel, "%02X %s", data[lineCount + i], (i == halfWay) ? " " : "");
            if (data[lineCount + i] >= ' ' && data[lineCount + i] <= '~') {
                textPart[i] = data[lineCount + i];
            } else {
                textPart[i] = '.';
            }
        }
        logger(logLevel, ":  %s", textPart);
        logger(logLevel, "\n");
    }
}

void dmkFindSectorLocation(DMK_SectorDescriptor_t *tSectorParams, DMK_SectorDescriptor_t *dmkImageIndex) {
    int i;
    tSectorParams->sectorLocation = 0;
    for (i = 0; i < DMK_II_SECTORMAX; i++) {
        if (tSectorParams->track == dmkImageIndex[i].track && 
                tSectorParams->sector == dmkImageIndex[i].sector && tSectorParams->side == dmkImageIndex[i].side) {
            logger(TRACE, "Found tr/sd/sc %d/%d/%d at %05X, index:%d\n", dmkImageIndex[i].track, dmkImageIndex[i].side, 
                    dmkImageIndex[i].sector, dmkImageIndex[i].sectorLocation, i);
            tSectorParams->sectorLocation = dmkImageIndex[i].sectorLocation;
            tSectorParams->flags          = dmkImageIndex[i].flags;
            return;
        }
    }
}

int checkCRC(unsigned char *data, int size, unsigned short crc, int doubleDensity) {
    int i;
    unsigned short myCRC = (doubleDensity) ? 0xCDB4 : 0xFFFF;
    
    for (i = 0; i < size; i++) {
        myCRC = calc_crc(myCRC, data[i]);
    }
    return (crc == myCRC);
}

unsigned short dmkFindDAM(unsigned char *data, int size, int doubleDensity, int startOffset) {
    int i, j;
    unsigned short DAMlocation = 0;
    if (doubleDensity) {
        return findOneOfThem(data, size, startOffset, &valid_MFM_DAMs[0], VALID_MFM_IDAM_LIST_SIZE);
    } else {
        return findOneOfThem(data, size, startOffset, &valid_FM_DAMs[0], VALID_FM_IDAM_LIST_SIZE);
    }
    return 0;
}

unsigned short findOneOfThem(unsigned char *data, int dataSize, int startOffset, unsigned char *validValues, int validValuesSize) {
    int i, j;
    for (i = startOffset; i < dataSize; i++) {
        for (j = 0; j < VALID_FM_IDAM_LIST_SIZE; j++) {
            if (data[i] == valid_FM_DAMs[j]) {
                logger(BYTES, "\nDD DAM: %02X found at %d\n", valid_FM_DAMs[j], i) ;
                return i;
            }
        }
    }
    return 0;
}

void dmkUnDoubler(unsigned char *doubledData, unsigned char *singledData, int size, int interval) {
    int i;
    if (interval < 1) interval = 1;
    if (interval >  2) interval = 2;
    int range = size / interval;
    int offset = interval - 1;
    
    for (i = 0; i < range; i++) {
        singledData[i] = doubledData[i * interval + offset];
//        logger(BYTES, "%02X ", singledData[i]);
    }
//    logger(BYTES, "\n");  
    logBinaryBlock(BYTES, singledData, range);
}

int openImage(userParameters_t *tuserParameters) {
    logger(DEBUG, "Opening %s\n", tuserParameters->imageFile);
    tuserParameters->fileHandle = fopen(tuserParameters->imageFile, "rb");
    if (tuserParameters->fileHandle == NULL) {
        logger(ERROR, "Some error with opening %s. Exiting...\n", tuserParameters->imageFile);
        perror(tuserParameters->imageFile);
        return(0);
    }
    logger(DEBUG, "Image file: \'%s\', opened ok.\n", tuserParameters->imageFile);
    return(1);
}

void closeImage(userParameters_t* tuserParameters) {
    fclose(tuserParameters->fileHandle);
    tuserParameters->fileHandle = NULL;
}

void shutdown(userParameters_t* tuserParameters) {
    if (tuserParameters->fileHandle != NULL) {
        closeImage(tuserParameters);
    }
}

int charArrCheck(char *data1, char *data2, int size) {
    int i;
    for (i = 0; i < size; i++) {
        if (data1[i] != data2[i]) {
            return 0;
        }
    }
    return 1;
}

int getImageSize(userParameters_t* tuserParameters) {
    fseek(tuserParameters->fileHandle, 0, SEEK_END);
    return ftell(tuserParameters->fileHandle);
}

void jv3Flags(unsigned char flags) {
    int density = 0;
    logger(DEBUG, "(");
    if ((int)flags & JV3_DENSITY) {
        logger(DEBUG, "DD, ");
        density = 1;
    } else {
        logger(DEBUG, "SD, ");
    }
    if ((int)flags & JV3_SIDE) {
        logger(DEBUG, "side 1, ");
    } else {
        logger(DEBUG, "side 0, ");
    }
    if (density) { // Double Density
        if ((flags & JV3_DAM) == DD_0xFB_DAM) logger(DEBUG, "DAM: 0xFB (Normal), ");
        if ((flags & JV3_DAM) == DD_0xF8_DAM) logger(DEBUG, "DAM: 0xF8 (Deleted), ");
        if ((flags & JV3_DAM) == DD_0x40_DAM) logger(DEBUG, "DAM: Invalid; unused, ");
        if ((flags & JV3_DAM) == DD_0x60_DAM) logger(DEBUG, "DAM: Invalid; unused), ");        
    } else { // Single Density
        if ((flags & JV3_DAM) == SD_0xFB_DAM) logger(DEBUG, "DAM: 0xFB (Normal), ");
        if ((flags & JV3_DAM) == SD_0xFA_DAM) logger(DEBUG, "DAM: 0xFA (User-defined), ");
        if ((flags & JV3_DAM) == SD_0xF9_DAM) logger(DEBUG, "DAM: 0xF9 (Mod I dir), ");
        if ((flags & JV3_DAM) == SD_0xF8_DAM) logger(DEBUG, "DAM: 0xF8 (Deleted), ");
    }
    if ((int)flags & JV3_ERROR) logger(DEBUG, "crc error, ");
    
    if ((int)flags & JV3_NONIBM) logger(DEBUG, "non IBM (short), ");
    
    if (((int)flags & JV3_SIZE) == 0) logger(DEBUG, "256 bytes");
    if (((int)flags & JV3_SIZE) == 1) logger(DEBUG, "128 bytes");
    if (((int)flags & JV3_SIZE) == 2) logger(DEBUG, "1024 bytes");
    if (((int)flags & JV3_SIZE) == 3) logger(DEBUG, "512 bytes");
    logger(DEBUG, ")\n");
}

static int threshold = 1;

void setLogLevel(int level) {
    threshold = level;
}

void logger(int level, const char *fmt, ...)
{
  va_list args;
  if (level <= threshold) {
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
  }
}

int readFromImage(void *data, long location, int size, userParameters_t *tuserParameters) {
    int result;
    
    result = fseek(tuserParameters->fileHandle, location, SEEK_SET);
    if (result != 0) {
        logger(ERROR, "Error during fseek on file: got %d\n", result);
        return(0);        
    }
    
    result = fread(data, sizeof(char), size, tuserParameters->fileHandle);    
    if (result != size) {
        logger(TRACE, "Error reading bytes file: got %d, not %d\n", result, size);
        return(0);
    }
    
    unsigned char *myData = data;
    
    #define LOCALMAX 600
    int mySize = size < LOCALMAX ? result : LOCALMAX;
    logBinaryBlock(BYTES, myData, mySize); 
    
/*
    int i;
    int mySize = size < LOCALMAX ? size : LOCALMAX;
    for (i = 0; i < mySize; i++) {
        logger(BYTES, "%02X ", myData[i]);
    }
    logger(BYTES, "\n");
*/

    return 1;
}