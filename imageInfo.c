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

#include "imageFile.h"
#include "imageIndex.h"
#include "logger.h"
#include "crc.h"
#include "imageSignatures.h"
#include "trsdos.h"
#include "newdos.h"
#include "jv3.h"
#include "dmk.h"
#include "imageInfo.h"

/* ImageInfo -  Analyse disk images for typical TRS-80  formats, being JV1, JV3 and DMK
 *              The tool builds a format independent index of the sectors in the image, 
 *              so higher level functions need not know the actual format used. The
 *              getSector() function hides the format dependent aspects. 
 * 
 * fjkraan, 2012-12-22
 */

int main(int argc, char** argv) {
    userParameters_t ups;
    userParameters_t *userParameters = &ups;
    userParameters->errorMessage = "";
    userParameters->fileHandle = NULL;
    unsigned char dt[TRSDOS_SECTOR_SIZE];    
    unsigned char *data = &dt[0];

    SectorDescriptor_t sp;
    SectorDescriptor_t *sectorParams = &sp;
    SectorDescriptor_t sd[II_SECTORMAX];
    SectorDescriptor_t *imageIndex = &sd[0];
    ImageProperties_t dp;
    ImageProperties_t *imageProperties = &dp;
    int directoryTrack;
    int sectorsPerTrack;
    int isValidImage;

    optionParse(argc, argv, userParameters);
    
    if (!openImage(userParameters)) {
        logger(ERROR, "Error opening file '%s'", userParameters->imageFile);
        exit(1);
    }
    
    cleanImageIndex(imageIndex);
    isValidImage = jv1Check(userParameters, imageIndex);
    if (isValidImage) {
        checkImage(imageIndex, userParameters);
        exit(0);
    }
    
    cleanImageIndex(imageIndex);   
    isValidImage = jv3Check(userParameters, imageIndex);
    if (isValidImage) {
        checkImage(imageIndex, userParameters);
        exit(0);
    }
    
    cleanImageIndex(imageIndex);
    isValidImage = dmkCheck(userParameters, imageIndex);
    if (isValidImage) {
        checkImage(imageIndex, userParameters);
        exit(0);
    }

    logger(WARN, "Image type '%s' not recognised\n", userParameters->imageFile);
    
    closeImage(userParameters);
    
    exit(0);
}

int checkImage(SectorDescriptor_t *imageIndex, userParameters_t *tuserParameters) {
    ImageProperties_t dp;
    ImageProperties_t *imageProperties = &dp;
    SectorDescriptor_t sp;
    SectorDescriptor_t *sectorParams = &sp;
    unsigned char dt[TRSDOS_SECTOR_SIZE];    
    unsigned char *data = &dt[0];
    int sectorsPerTrack;
    int directoryTrack;
    
    getDiskProperties(imageProperties, imageIndex);
    sectorsPerTrack = imageProperties->track1SectorCount;
    sectorParams->track  = 0;
    sectorParams->sector = 0;
    sectorParams->side   = 0;
    getSectorLocation(sectorParams, imageIndex);
    getSector(data, sectorParams, tuserParameters);

    directoryTrack = checkBootSignature(data, bootSectorSignature_trsdos23);
    if (directoryTrack != 0) {
        logger(INFO, "Boot sector looks like TRSDOS 2.x or similar DOS\n");
        
        sectorParams->track  = 0;
        sectorParams->sector = 2;
        sectorParams->side   = 0;
        getSectorLocation(sectorParams, imageIndex);
        getSector(data, sectorParams, tuserParameters);
        if (logPDRIVE(INFO, data)) {
            logger(INFO, "Disk has a valid NewDos/80 PDRIVE table\n");
        }
        
        if (!tuserParameters->showDirectory) return(0);
        sectorParams->track = directoryTrack;
        showDir(sectorParams, imageIndex, tuserParameters);
        return(1);
    }
    directoryTrack = checkBootSignature(data, bootSectorSignature_dosplus);
    if (directoryTrack != 0) {
        logger(INFO, "Boot sector looks like DOSPlus\n");
        if (!tuserParameters->showDirectory) return(0);
        sectorParams->track = directoryTrack;
        showDir(sectorParams, imageIndex, tuserParameters);
        return(1);
    }
    directoryTrack = checkBootSignature(data, bootSectorSignature_trsdos27);
    if (directoryTrack != 0) {
        logger(INFO, "Boot sector looks like TRSDOS 2.7\n");
        if (!tuserParameters->showDirectory) return(0);
        sectorParams->track = directoryTrack;
        showDir(sectorParams, imageIndex, tuserParameters);
        return(1);
    }
    directoryTrack = checkBootSignature(data, bootSectorSignature_trsdos13);
    if (directoryTrack != 0) {
        logger(INFO, "Boot sector looks like TRSDOS 1.3\n");
        if (!tuserParameters->showDirectory) return(0);
        sectorParams->track = directoryTrack;
        showDir(sectorParams, imageIndex, tuserParameters);
        return(1);
    }
    directoryTrack = checkBootSignature(data, bootSectorSignature_loboLdos);
    if (directoryTrack != 0) {
        logger(INFO, "Boot sector looks like Lobo LDOS\n");
        if (!tuserParameters->showDirectory) return(0);
        sectorParams->track = directoryTrack;
        showDir(sectorParams, imageIndex, tuserParameters);
        return(1);
    }
    logger(INFO, "Boot track:\n");
    logBinaryBlock(INFO, data, TRSDOS_SECTOR_SIZE);
    return(0);
}

int getSector(unsigned char *data, SectorDescriptor_t *sectorParams, 
        userParameters_t *tuserParameters) {
//    getSector(unsigned char *data, int location, int size, int doubleBytes, userParameters_t *tuserParameters) {
    unsigned char rdt[MAX_SECTOR_SIZE * 2];
    unsigned char *rawData = &rdt[0];
    int doubleBytes = sectorParams->flags & DMK_DATA_DOUBLER_MASK;
    int location    = sectorParams->sectorLocation;
    
    int size, interval;
    
    if (doubleBytes) {
        size = TRSDOS_SECTOR_SIZE * 2;
        interval = 2;
    } else {
        size = TRSDOS_SECTOR_SIZE;
        interval = 1;
    }
    
    logger(TRACE, "Found track %d, sector %d, side %d, fl %02X at location %05X\n", 
            sectorParams->track, sectorParams->sector, sectorParams->side, sectorParams->flags, location);
    
    readFromImage(rawData, location, size, tuserParameters);
    dataUnDoubler(rawData, data, size, interval);
    logger(TRACE, (doubleBytes) ? "sector reduced:\n" : "sector copied:\n");

    return 0;
}

int checkBootSignature(unsigned char *data, unsigned short bootSectorSignature[]) {
    int i, directoryTrack = 0;
    int signatureSize = 0;
    unsigned short shortedData;
    
    while (signatureSize < TRSDOS_SECTOR_SIZE && bootSectorSignature[signatureSize] < BOOTSECTORSIGNATURE_TERMINATOR) {
        signatureSize++;
    }
    for (i = 0; i < signatureSize; i++) {
        logger(TRACE, "Check sig - %d: %02X == %02X\n", i, data[i], bootSectorSignature[i]);
        shortedData = data[i];
        if (bootSectorSignature[i] == BOOTSECTORSIGNATURE_IGNORE) continue;
        if (bootSectorSignature[i] == BOOTSECTORSIGNATURE_DIR_TRACK) {
            logger(TRACE, "Directory track: %d\n", shortedData);
            directoryTrack = shortedData;
            continue;
        }
        if (data[i] != bootSectorSignature[i]) {
            logger(TRACE, "Boot sector signature mismatch\n");
            return 0;
        }
    }
    return directoryTrack;
}

#define MAX_LOG_LEVEL 5
void optionParse(int argc, char** argv, userParameters_t *tuserParameters) {
    if (argc <= 1) {
        tuserParameters->errorMessage = "no arguments";
        usage(tuserParameters);
        exit(1);
    }
    tuserParameters->showDirectory = 0;
    const char *optstr = "df:hsv:?";
    char ch;
    int level;
    if (argc <= 1) usage(tuserParameters);
    while( -1 != (ch=getopt(argc,argv,optstr))) {
        switch(ch) {
            case 'd':
                tuserParameters->showDirectory = 1;
                break;
            case 'f':
                tuserParameters->imageFile = optarg;
                break;                
            case 'h':
                usage(tuserParameters);
                exit(1);
                break;
            case 's':
                tuserParameters->forceSingleSided = 1;
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
    logger(ERROR, "imageInfo version 0.8\n");
    if (strncmp(tuserParameters->errorMessage, "", 1) != 0) {
        logger(ERROR, "%s\n", tuserParameters->errorMessage);
    }
    logger(ERROR, "usage: -d display directory tracks\n");
    logger(ERROR, "       -f <image file>\n");
    logger(ERROR, "       -h              - help and version\n");
    logger(ERROR, "       -s force single sided interpretation\n");
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
int jv1Check(userParameters_t *tuserParameters, SectorDescriptor_t *imageIndex) {
    logger(INFO, "JV1 check:\n");
    int size = getImageSize(tuserParameters);
    unsigned char data[TRSDOS_SECTOR_SIZE];
    unsigned char directoryTrack;
    int maxTrack;
    int track, sector, i;
    SectorDescriptor_t sp;
    SectorDescriptor_t *sectorParams = &sp;
    
    if (size == JV1_SSSD35_SIZE) {
        logger(INFO, " Size is correct for a 35 tracks, 10 sectors of 256 bytes JV1 image\n");
        maxTrack = 35;
    } else if (size == JV1_SSSD40_SIZE) {
        logger(INFO, " Size is correct for a 40 tracks, 10 sectors of 256 bytes JV1 image\n");
        maxTrack = 40;
    } else if (size == JV1_SSSD80_SIZE) {
        maxTrack = 80;
        logger(INFO, " Size is correct for a 80 tracks, 10 sectors of 256 bytes JV1 image\n");
    } else {
        logger(INFO," Size of %d bytes does not match a standard JV1 image\n", size);
        return 0;
    }
    
    for (track = 0; track < maxTrack; track++) {
        for (sector = 0; sector < TRSDOS_SS_SD_SPT; sector++) {
            i = track * TRSDOS_SS_SD_SPT + sector;
            imageIndex[i].track  = track;
            imageIndex[i].sector = sector;
            imageIndex[i].side   = 0;
            imageIndex[i].sectorLocation = i * TRSDOS_SECTOR_SIZE;
            imageIndex[i].flags = SD_SIZE_256; // 256 bytes, single byte, single density
            logger(TRACE, "jv1Check: %d: Tr. %d, Sc. %d, Sd. %d @ %05X, Fl. %02X\n", i,
                    imageIndex[i].track,          imageIndex[i].sector, imageIndex[i].side,
                    imageIndex[i].sectorLocation, imageIndex[i].flags);
        }
    }

    sectorParams->track = 0;
    sectorParams->sector = 0;
    sectorParams->side = 0;
    sectorParams->flags = SD_SIZE_256;
    getSectorLocation(sectorParams, imageIndex);
    getSector(data, sectorParams, tuserParameters);
    directoryTrack = checkBootSignature(data, bootSectorSignature_trsdos23);

    logger(WARN, "Image is a ");
    logger(ERROR, "JV1\n");
    return directoryTrack;
}


// The JV3 does not really have a identifying header, but each track is preceeded 
// with a array describing the order and type of the sectors on that track.
// One check is iterating over these arrays for all tracks and count the maximum 
// track, side and sector number is a simple check. If the track and sector 
// count is not inside a reasonable range, the image isn't a valid JV3. 
// Another task of this function is building an sectorIndex, containing the location
// of each sector. Together with the JV3 sector header, copied from the image, 
// this can be used to retrieve the sector from the image.
int jv3Check(userParameters_t *tuserParameters, SectorDescriptor_t *imageIndex) {
    int result;
    unsigned char track, sector, side, flags;
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
    JV3SectorHeader_t jv3H1[JV3_SECTORS]; 
    JV3SectorHeader_t *jv3Headers1 = &jv3H1[0];
    unsigned char wpFlag[1];
    
    logger(INFO, "JV3 check:\n");
    
    result = readFromImage(jv3Headers1, 0, headerBlockSize, tuserParameters);
    
    int i;
    for (i = 0; i < JV3_SECTORS; i++) {
        track  = jv3Headers1[i].track;
        sector = jv3Headers1[i].sector;
        side   = (jv3Headers1[i].flags & JV3_SIDE) ? 1 : 0;
        if (tuserParameters->forceSingleSided && side) continue; // 
        flags  = jv3Headers1[i].flags;
        if (track == 255 || sector == 255) { // This indicates no further sectors are in this header.
            continue;
        } else {
            trackHighCount = track > trackHighCount ? track : trackHighCount;
            sectorHighCount = sector > sectorHighCount ? sector : sectorHighCount;
            sides  = ((int)flags & JV3_SIDE) ? 2 : 1;
            if (track == 0 && sector == 0 && ((int)flags & JV3_SIDE) == 0) { bootSectorIndex = i; }
        }        
        imageIndex[i].track  = jv3Headers1[i].track;
        imageIndex[i].side   = side;
        imageIndex[i].sector = jv3Headers1[i].sector;
        imageIndex[i].flags  = jv3TranslateSizes(jv3Headers1[i].flags & JV3_SIZE);
        imageIndex[i].flags += (jv3Headers1[i].flags & JV3_DENSITY) ? JV3I_DOUBLE_DENSITY : 0;
        
        previousTrack = track;
        if (((int)flags & JV3_SIZE) == 0) sectorSize = 256;
        if (((int)flags & JV3_SIZE) == 1) sectorSize = 128;
        if (((int)flags & JV3_SIZE) == 2) sectorSize = 1024;
        if (((int)flags & JV3_SIZE) == 3) sectorSize = 512;
        imageIndex[i].sectorLocation = previousOffset + previousSectorSize;
        previousOffset = imageIndex[i].sectorLocation;
        previousSectorSize = sectorSize;
        logger(TRACE, " %d", i);
        logger(TRACE, " (@ %05X)", imageIndex[i].sectorLocation);
        logger(TRACE, ", tr: %d, sc: %d, fl: %02X ", track, sector, flags);
        jv3Flags(TRACE, flags);
        logger(TRACE, "\n");
    }
    logger(DEBUG, " Highest track: %d, Sides: %d, ", trackHighCount, (side) ? 2 : 1);
    result = readFromImage(&wpFlag, wpFlagLocation, 1, tuserParameters); // the next byte after headers1
    logger(DEBUG, "(0x%02X) @ %05X: ", wpFlag[0], wpFlagLocation);
    if (wpFlag[0] == JV3_WPFLAG_OFF) {
        logger(DEBUG, "Write Enabled ");
    } else {
        logger(DEBUG, "Write Protected ");
    }
        
    if (trackHighCount >= MINIMUM_TRACKS && trackHighCount < MAXIMUM_TRACKS && sectorHighCount < MAX_SECTORS_PER_TRACK) {
        logger(WARN, "Image is a ");
        logger(ERROR, "JV3");
        logger(WARN, " image");
        logger(ERROR, "\n");
        return 1; // only sane values found for a normal disk.
    }
    logger(INFO, " Image does not match a JV3 profile\n");
    return 0;
}

unsigned char jv3TranslateSizes(unsigned char jv3HeaderSizes) {
    // from 0=256, 1=128, 2=1024, 3=512
    // to   0=128, 1=256, 3=512, 4=1024
    if (jv3HeaderSizes == 0) return 1;
    if (jv3HeaderSizes == 1) return 0;
    if (jv3HeaderSizes == 2) return 3;
    if (jv3HeaderSizes == 3) return 2;
    return 1;
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
int dmkCheck(userParameters_t *tuserParameters, SectorDescriptor_t *imageIndex) {
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
    if (!result) {
        logger(ERROR, "Failure reading from image. Exiting.");
        exit(0);
    }
    writeProtect = dmkImageHeader->writeProtect;
    singleDensitySingleByte = dmkImageHeader->options & DMK_SINGLE_DENSITY_SINGLE_BYTE;
    
    logger(DEBUG, " Tracks: %d, ", dmkImageHeader->tracks);
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
//    logger(WARN, "Image is a DMK image\n");
    logger(DEBUG, " (0x%04X), ", trackLength);
    logger(DEBUG, "%s Sided, ", (dmkImageHeader->options & DMK_SINGLE_SIDED_ONLY) ? "Single" : "Double" );
    if (writeProtect == 0xFF) {
        logger(DEBUG, "Write Protected ");
    } else if (writeProtect == 0x00) {
        logger(DEBUG, "Write Enabled ");
    } else {
        logger(INFO, ", unknown write state: 0x%02X (should be Write Protected (0xFF) or Write Enabled (0x00)) ", writeProtect);
    }
    char ignDens = dmkImageHeader->options & DMK_IGNORE_DENSITY;
    logger(TRACE, ", %s density (obsolete), ", 
            (ignDens == 1) ? "Ignore" : "Respect");
    logger(TRACE, "SD sector bytes are written %s\n", (singleDensitySingleByte) ? "once" : "twice");
    
    crcCheckErrors = dmkImageParser(imageIndex, dmkImageHeader, tuserParameters);
    
    if (crcCheckErrors == 0) {
        logger(DEBUG, " All CRCs ok");
    } else {
        logger(DEBUG, " %d CRC errors (IDAM & DAM)", crcCheckErrors);
    }
    logger(DEBUG, ".\n");

    if (!checkGeometricSanity(imageIndex)) return 0;
    return 1; // valid
}

int dmkImageParser(SectorDescriptor_t *imageIndex, 
        DMKImageHeader *dmkImageHeader, userParameters_t *tuserParameters) {
    int i = 0, result, j = 0;
    int trackCount = 0;
    int crcCheckErrors = 0;
    
    DMKTrackHeader dt1h;
    DMKTrackHeader *dmkTrackHeader = &dt1h;
    unsigned char sd[DMK_RAW_SECTOR_SIZE];
    unsigned char *sectorData = &sd[0];

    int doubleDensity = 0;
    unsigned short sectorPointer;
    unsigned short nextSectorPointer;
    unsigned short rawSectorPointer;
    unsigned char ssd[DMK_RAW_SECTOR_SIZE];
    unsigned char *singledSectorData = &ssd[0];
    int imageLevelSingleByte = dmkImageHeader->options & DMK_SINGLE_DENSITY_SINGLE_BYTE;
    int sectorLevelSingleByte = imageLevelSingleByte;
    unsigned short trackLength = dmkImageHeader->trackLengthLSB + 
        dmkImageHeader->trackLengthMSB * 256;
    logger(DEBUG, "DMK header track count: %d\n", dmkImageHeader->tracks);

    for (trackCount = 0; trackCount < dmkImageHeader->tracks; trackCount++) {
        long trackHeaderLocation = sizeof(DMKImageHeader) + trackLength * trackCount;
        logger(TRACE, "---- Track %d: %05X ----\n", trackCount, trackHeaderLocation);
        result = readFromImage(dmkTrackHeader, trackHeaderLocation, sizeof(DMKTrackHeader), tuserParameters);

        for (i = 0; i < DMK_SECTOR_POINTERS_IN_HEADER; i++) {
            sectorPointer = dmkTrackHeader->revRpointer[i].sectorPointerLSB + 
                    (dmkTrackHeader->revRpointer[i].sectorPointerMSB & DMK_SECTOR_POINTER_MSB_MASK) * 256;
            rawSectorPointer = dmkTrackHeader->revRpointer[i].sectorPointerLSB + 
                    dmkTrackHeader->revRpointer[i].sectorPointerMSB * 256;
            doubleDensity = (dmkTrackHeader->revRpointer[i].sectorPointerMSB & DMK_DOUBLE_DENSITY_SECTOR);
            sectorLevelSingleByte = dmkTrackHeader->revRpointer[i+1].sectorPointerMSB & DMK_DOUBLE_DENSITY_SECTOR;
            if (i < DMK_SECTOR_POINTERS_IN_HEADER - 1) {
                nextSectorPointer = dmkTrackHeader->revRpointer[i+1].sectorPointerLSB + 
                    (dmkTrackHeader->revRpointer[i+1].sectorPointerMSB & 0x3F) * 256; 
            }
            if (sectorPointer == 0) {
                logger(TRACE, "highest sector is %d\n", i);
                break;
            }
            logger(TRACE, "Sector header pointer(0x%04X + 0x%05X): 0x%04X, Density: %s - ", 
                    sectorPointer, trackHeaderLocation, sectorPointer + trackHeaderLocation, (doubleDensity) ? "double" : "single");
            readFromImage(sectorData, sectorPointer + trackHeaderLocation, DMK_RAW_SECTOR_SIZE, tuserParameters);
            if (doubleDensity) { // The double byte issue is only for single density
                logger(TRACE, "Copying single byte DD sector\n");
                dataUnDoubler(sectorData, singledSectorData, DMK_RAW_SECTOR_SIZE, 1);
            } else {
                if (sectorLevelSingleByte) { // 
                    logger(TRACE, "Copying single byte SD sector\n");
                    dataUnDoubler(sectorData, singledSectorData, DMK_RAW_SECTOR_SIZE, 1);
                } else {
                    logger(TRACE, "Reducing double byte SD sector\n");
                    dataUnDoubler(sectorData, singledSectorData, DMK_RAW_SECTOR_SIZE, 2);
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
//            logger(TRACE, "(Loc Data: 0x%05X)\n", 
//                    trackHeaderLocation + sectorPointer + dmkRawSector[DMK_RAW_SECTOR_INDEX_DAT]);

            dmkIndexSectorData(singledSectorData, &dmkRawSector[0], &imageIndex[j]); 
            crcCheckErrors += dmkGetCRCErrorScore(singledSectorData, &dmkRawSector[0], doubleDensity);

            long sectorDataLocation = 
                trackHeaderLocation + sectorPointer + dmkRawSector[DMK_RAW_SECTOR_INDEX_DAT];
            imageIndex[j].sectorLocation = sectorDataLocation;
            if (doubleDensity) {
                imageIndex[j].flags += SD_DOUBLE_DENSITY;
            } else {
                imageIndex[j].flags += SD_DATA_DOUBLER_MASK;
            }
//            imageIndex[j].flags += (sectorLevelSingleByte && !doubleDensity) ? DMK_DATA_DOUBLER_MASK : 0;
//            imageIndex[j].flags += (!sectorLevelSingleByte && doubleDensity) ? DMK_DATA_DOUBLER_MASK : 0;
            logger(TRACE, "---- Tr.%d, Sd.%d, Sc.%d db.%02X @ 0x%06X ----\n", 
                    imageIndex[j].track, imageIndex[j].side, 
                    imageIndex[j].sector, imageIndex[j].flags, imageIndex[j].sectorLocation);
            j++;
        }
    }
    return crcCheckErrors;
}

void dmkIndexSectorData(unsigned char *data, int *sectorIndex, SectorDescriptor_t *imageIndex) {
    unsigned char idam     = data[sectorIndex[DMK_RAW_SECTOR_INDEX_ID]];
    unsigned char track    = data[sectorIndex[DMK_RAW_SECTOR_INDEX_TR]];
    unsigned char side     = data[sectorIndex[DMK_RAW_SECTOR_INDEX_SD]];
    unsigned char sector   = data[sectorIndex[DMK_RAW_SECTOR_INDEX_SC]];
    unsigned char size     = data[sectorIndex[DMK_RAW_SECTOR_INDEX_EN]];
    unsigned char dam      = data[sectorIndex[DMK_RAW_SECTOR_INDEX_DM]];
    
    logger(TRACE, "IDAM: %02X, ", idam);
    logger(TRACE, "Track: %d, Side: %d, Sector: %d, Size: %d, ", track, side, sector, 128<<size);
    logger(TRACE, "DAM: %02X, ", dam);
    
    imageIndex->track  = track;
    imageIndex->side   = side;
    imageIndex->sector = sector;
    imageIndex->sectorLocation = 0; // set in dmkTrackParser()
    imageIndex->flags  = size;      // other flag set in dmkTrackParser(), dmkFindSectorLocation()
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
        for (j = 0; j < validValuesSize; j++) {
            if (data[i] == validValues[j]) {
                logger(BYTES, "\nDAM: %02X found at %d\n", validValues[j], i) ;
                return i;
            }
        }
    }
    return 0;
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

void jv3Flags(int logLevel, unsigned char flags) {
    int density = 0;
    logger(logLevel, "(");
    if ((int)flags & JV3_DENSITY) {
        logger(logLevel, "DD, ");
        density = 1;
    } else {
        logger(logLevel, "SD, ");
    }
    if ((int)flags & JV3_SIDE) {
        logger(logLevel, "side 1, ");
    } else {
        logger(logLevel, "side 0, ");
    }
    if (density) { // Double Density
        if ((flags & JV3_DAM) == DD_0xFB_DAM) logger(logLevel, "DAM: 0xFB (Normal), ");
        if ((flags & JV3_DAM) == DD_0xF8_DAM) logger(logLevel, "DAM: 0xF8 (Deleted), ");
        if ((flags & JV3_DAM) == DD_0x40_DAM) logger(logLevel, "DAM: Invalid; unused, ");
        if ((flags & JV3_DAM) == DD_0x60_DAM) logger(logLevel, "DAM: Invalid; unused), ");        
    } else { // Single Density
        if ((flags & JV3_DAM) == SD_0xFB_DAM) logger(logLevel, "DAM: 0xFB (Normal), ");
        if ((flags & JV3_DAM) == SD_0xFA_DAM) logger(logLevel, "DAM: 0xFA (User-defined), ");
        if ((flags & JV3_DAM) == SD_0xF9_DAM) logger(logLevel, "DAM: 0xF9 (Mod I dir), ");
        if ((flags & JV3_DAM) == SD_0xF8_DAM) logger(logLevel, "DAM: 0xF8 (Deleted), ");
    }
    if ((int)flags & JV3_ERROR) logger(logLevel, "crc error, ");
    
    if ((int)flags & JV3_NONIBM) logger(logLevel, "non IBM (short), ");
    
    if (((int)flags & JV3_SIZE) == 0) logger(logLevel, "256 bytes");
    if (((int)flags & JV3_SIZE) == 1) logger(logLevel, "128 bytes");
    if (((int)flags & JV3_SIZE) == 2) logger(logLevel, "1024 bytes");
    if (((int)flags & JV3_SIZE) == 3) logger(logLevel, "512 bytes");
    logger(logLevel, ")");
}

void showDir(SectorDescriptor_t *sectorParams, SectorDescriptor_t *imageIndex, userParameters_t *userParameters) {
    unsigned char dt[TRSDOS_SECTOR_SIZE];    
    unsigned char *data = &dt[0];
    int sectorsPerTrack = getSectorCountTrackSide(sectorParams, imageIndex);

    sectorParams->side   = 0;
    sectorParams->sector = DIR_GAT_SECTOR;
    getSectorLocation(sectorParams, imageIndex);
    getSector(data, sectorParams, userParameters);

    getSectorLocation(sectorParams, imageIndex);
    getSector(data, sectorParams, userParameters);
    showGAT(INFO, data, TRSDOS_SECTOR_SIZE);

    sectorParams->sector = DIR_HIT_SECTOR;
    getSectorLocation(sectorParams, imageIndex);
    getSector(data, sectorParams, userParameters);
    showHIT(INFO, data, TRSDOS_SECTOR_SIZE, sectorsPerTrack);

    int d;
    logger(INFO, "\nDirectory sectors %d - %d\n", DIR_FIRST_ENTRY_SECTOR, sectorsPerTrack);
    logger(INFO, "Ac          Ov Ef Rl  Filename/Ext Hash UpPw AcPw EOFs   G1    G2    G3    G4  FXDE\n");
    for (d = DIR_FIRST_ENTRY_SECTOR; d < sectorsPerTrack; d++) {
        sectorParams->sector = d;
        getSectorLocation(sectorParams, imageIndex);
        getSector(data, sectorParams, userParameters);
//                logBinaryBlock(INFO, data, TRSDOS_SECTOR_SIZE); 
        logger(INFO, "Sector %d:\n", d);
        showDirSector(INFO, data, TRSDOS_SECTOR_SIZE);
    }
    
}

void showGAT(int logLevel, unsigned char *data, int size) {
#define GAT_TRACKS 80
#define GRANULES_PER_TRACK 2
#define SECTORS_PER_GRANULE 5
#define GAT_LOCKEDOUT_OFFSET 0x60
#define GAT_DISK_LABEL 0xD0
#define GAT_DISK_LABEL_SIZE 8
#define GAT_DISK_DATE  0xD8
#define GAT_DISK_DATE_SIZE 8

    char diskLabel[GAT_DISK_LABEL_SIZE + 1] = "        \0";
    char diskDate[GAT_DISK_DATE_SIZE + 1]   = "        \0";
    int i, j;
    int granuleLockout;
    int freeDiskSpace = 0;
    logger(logLevel, "\n");
    logger(logLevel, "GAT (# = used, _ = free):\n");
    for (i = 0; i < GAT_TRACKS; i++) {
        logger(logLevel, "%2d (%02X)", i, data[i]);
        for (j = 0; j < GRANULES_PER_TRACK; j++) {
            if (data[i] & (1<<j)) {
               logger(logLevel, "#");
            } else {
                logger(logLevel, "_");
                freeDiskSpace +=  SECTORS_PER_GRANULE * TRSDOS_SECTOR_SIZE;     
            }
        }
        logger (logLevel, "%s", (i%5 == 4) ? "\n" : " ");
    }
    logger(logLevel, "\n");
    logger(logLevel, "Free Disk space: %d bytes\n", freeDiskSpace);
    logger(logLevel, "Granule lockout table (# = locked out, _ = usable):\n");
    for (i = 0; i < GAT_TRACKS; i++) {
        granuleLockout = GAT_LOCKEDOUT_OFFSET + i;
           logger(logLevel, "%2d (%02X)", i, data[granuleLockout]);
        for (j = 0; j < GRANULES_PER_TRACK; j++) {
            if (data[granuleLockout] & (1<<j)) {
               logger(logLevel, "#");
            } else {
                logger(logLevel, "_");
                freeDiskSpace +=  SECTORS_PER_GRANULE * TRSDOS_SECTOR_SIZE;     
            }
        }
        logger (logLevel, "%s", (i%5 == 4) ? "\n" : " ");
    }
    logger(logLevel, "\n");
    logger(logLevel, "Disk label & date: ");
    memcpy(diskLabel, &data[GAT_DISK_LABEL], GAT_DISK_LABEL_SIZE);
    memcpy(diskDate,  &data[GAT_DISK_DATE],  GAT_DISK_DATE_SIZE);
    logger(logLevel, "%s %s\n", diskLabel, diskDate);
}

void showHIT(int logLevel, unsigned char *data, int size, int sectorsPerTrackSide) {
#define HASH_REGION_SIZE     0x08
#define HASH_REGION_INTERVAL 0x20
#define HASH_SIZE            0x02
#define HASH_COUNT           0x08
#define FIRST_ENTRY_SECTOR   0x02
    
    int i, j;
    int directoryEntrySectors = sectorsPerTrackSide - FIRST_ENTRY_SECTOR;
    int regionCount;
    logger(logLevel, "HIT file hashes:\n");
//    logger(logLevel, "S2 S3  4  5  6  7  8  9\n");
    for (j = FIRST_ENTRY_SECTOR; j < sectorsPerTrackSide; j++) logger(logLevel, "%2d ", j);
    logger(logLevel, "\n");
    for (regionCount = 0; regionCount < HASH_COUNT; regionCount++) {
//        logger(logLevel, "Sector %d: ", FIRST_ENTRY_SECTOR + regionCount);
        for (i = 0; i < directoryEntrySectors; i++) {
            logger(logLevel, "%02X ", data[i + regionCount * HASH_REGION_INTERVAL]);
        }
        logger(logLevel, "\n");
    }
}

void showDirSector(int logLevel, unsigned char *data, int size) {
    
    directoryEntry_t de;
    directoryEntry_t *directoryEntry = &de;
    char fileName[DIR_ENTRY_NAME_SIZE + 1] = "        \0";
    char fileExtension[DIR_ENTRY_EXT_SIZE + 1] = "   \0";
    char fileNameExt[DIR_ENTRY_NAME_SIZE + DIR_ENTRY_EXT_SIZE + 1] = "           \0";
    int i;
    
    for (i = 0; i < DIR_ENTRIES_PER_SECTOR; i++) {
        memcpy(directoryEntry, &data[DIR_ENTRY_INTERVAL * i], DIR_ENTRY_INTERVAL);
        memcpy(&fileName,      directoryEntry->fileName,      DIR_ENTRY_NAME_SIZE);
        memcpy(&fileExtension, directoryEntry->fileExtension, DIR_ENTRY_EXT_SIZE);
        memcpy(&fileNameExt, directoryEntry->fileName, DIR_ENTRY_NAME_SIZE);
        memcpy(&fileNameExt[DIR_ENTRY_NAME_SIZE], directoryEntry->fileExtension, DIR_ENTRY_EXT_SIZE);
        if (directoryEntry->fileName[0] != 0x00) {
//                logger(logLevel, "%s ", ((i < DIR_RESERVED_ENTRY_MAX) ? "S" : " "));
                logger(logLevel, "%s", (directoryEntry->accessControl & DIR_SECONDARY_DIRREC) ? "s" : ".");
                logger(logLevel, "%s", (directoryEntry->accessControl & DIR_SYSTEM_FILE) ?      "s" : ".");
                logger(logLevel, "%s", (directoryEntry->accessControl & DIR_SYSTEM_FILE) ?      "u" : ".");
                logger(logLevel, "%s ", (directoryEntry->accessControl & DIR_FILE_INVISIBLE) ?   "i" : ".");
                logger(logLevel, "%s ", daAccess[directoryEntry->accessControl & DIR_FILE_ACCESS_MASK]);
//                logger(logLevel, "%s ", (directoryEntry->accessControl & DIR_AC_SYSTEM_MASK) ? "S" : " ");
                logger(logLevel, "%02X ", directoryEntry->overflow);
                logger(logLevel, "%02X ", directoryEntry->EOFbyteOffset);
                logger(logLevel, "%02X  ", directoryEntry->recordLength);
                logger(logLevel, "%s/%s (%02X) ", fileName, fileExtension, calcHash(&fileNameExt[0], DIR_ENTRY_NAME_SIZE + DIR_ENTRY_EXT_SIZE));
                logger(logLevel, "%02X%02X ", directoryEntry->updatePasswordMSB, directoryEntry->updatePasswordLSB);
                logger(logLevel, "%02X%02X ", directoryEntry->accessPasswordMSB, directoryEntry->accessPasswordLSB);
                logger(logLevel, "%02X%02X ", directoryEntry->EOFsectorMSB, directoryEntry->EOFsectorLSB);
                logger(logLevel, "%2d ", directoryEntry->gap1.track);
                logger(logLevel, "%2d ", directoryEntry->gap1.numberOfGranules);
                if (directoryEntry->gap2.track == 0xFF) { logger(logLevel, "\n"); continue; }
                logger(logLevel, "%2d ", directoryEntry->gap2.track);
                logger(logLevel, "%2d ", directoryEntry->gap2.numberOfGranules);
                if (directoryEntry->gap3.track == 0xFF) { logger(logLevel, "\n"); continue; }
                logger(logLevel, "%2d ", directoryEntry->gap3.track);
                logger(logLevel, "%2d ", directoryEntry->gap3.numberOfGranules);
                if (directoryEntry->gap4.track == 0xFF) { logger(logLevel, "\n"); continue; }
                logger(logLevel, "%2d ", directoryEntry->gap4.track);
                logger(logLevel, "%2d ", directoryEntry->gap4.numberOfGranules);
                if (directoryEntry->FXDEflag != 0xFF) {
                    logger(logLevel, "%d,", (directoryEntry->FXDEflag & DIR_FXDE_ENTRY_MASK)>>5);
                    logger(logLevel, "%d ", directoryEntry->FXDEflag & DIR_FXDE_SECTOR_MASK);
                    logger(logLevel, "%d ", directoryEntry->FXDElocation);
                }
//                logger(logLevel, " %s/%s (%02X)\n", fileName, fileExtension, calcHash(fileName, DIR_ENTRY_NAME_SIZE + DIR_ENTRY_EXT_SIZE));
              logger(logLevel, "\n");
        }
    } 
}



unsigned char calcHash(char *fileName, int size) {
    int b;
    unsigned char a;
    int c = 0;
    int carry;
    logger(TRACE, " calcHash: '%s' (%d)\n", fileName, size);
    
    for (b = 0; b < size; b++) {
        a = fileName[b];
        a = a ^ c;
        carry = (a & 0x80);
        a = a<<1;
        a = (carry) ? a + 1 : a;
        c = a;
    }
    a = (a) ? a : 1;
    return a;
}
/*
      LD    B,11
      LD    C,0
LOOP  LD    A,(DE)
      INC   DE
      XOR   C
      RLCA
      LD    C,A
      DJNZ  LOOP
      LD    A,C
      OR    A
      JMP   DONE
      INC   A
DONE
        
        */


