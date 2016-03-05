/* 
 * File:   imageIndex.c
 * Author: fjkraan
 *
 * Created on March 26, 2014, 8:58 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "imageIndex.h"
#include "logger.h"



// Cleans the imageIndex. As the JV1 data starts at location 0, special values
// for track, side and sector are used to mark the unused entries.
void cleanImageIndex(SectorDescriptor_t *imageIndex) {
    int i;
    
    for (i = 0; i < II_SECTORMAX; i++) {
        imageIndex[i].track  = 255;
        imageIndex[i].sector = 255;
        imageIndex[i].side   = 255;
        imageIndex[i].flags  = 0;
        imageIndex[i].sectorLocation = 0;
    }
}

// Parses the imageIndex and checks track, side and sector values against reasonable
// maximum values. Returns false if any is out of bounds. Stops on track and sector 
// value == 255 which means not used..
int checkGeometricSanity(SectorDescriptor_t *imageIndex) {
    int ii;
    int trackHighCount = 0;
    int sectorHighCount = 0;
    int sideHighCount = 0;
    for (ii = 0; ii < II_SECTORMAX; ii++) {
        if (imageIndex[ii].track == 255 && imageIndex[ii].sector == 255) break;
        logger(BYTES, "Index: %d; Tr: %d; Sd: %d; Sc: %d; Location: %05X; Flags %X\n", 
                ii,
                imageIndex[ii].track, 
                imageIndex[ii].side, 
                imageIndex[ii].sector, 
                imageIndex[ii].sectorLocation, 
                imageIndex[ii].flags);
        if (imageIndex[ii].track > trackHighCount) trackHighCount = imageIndex[ii].track;
        if (imageIndex[ii].sector > sectorHighCount) sectorHighCount = imageIndex[ii].sector;
        if (imageIndex[ii].side > sideHighCount) sideHighCount = imageIndex[ii].side;
    }
    logger(TRACE, "Track max: %d, Side max: %d, Sector max: %d\n", trackHighCount, sideHighCount, sectorHighCount);
    
    if (trackHighCount >= MINIMUM_TRACKS && 
            trackHighCount < MAXIMUM_TRACKS && 
            sectorHighCount < MAX_SECTORS_PER_TRACK && 
            sideHighCount < MAXIMUM_SIDES) {
        logger(DEBUG, " Image is ");
        logger(DEBUG, "%s Sided ", (imageIndex[ii].side > 0) ? "Double" : "Single");
        logger(DEBUG, " with %d tracks and %d sectors (MAX) per track.\n", trackHighCount + 1, sectorHighCount + 1);
        return 1; // only sane values found for a normal disk.
    }
    return 0; // insane values
}

// Retrieves some meta data from the imageIndex like highest track number and highest sector number
// This for track 0 and all other tracks. Stops on track and sector value == 255 which means not used.
void getDiskProperties(ImageProperties_t *diskProperties, SectorDescriptor_t *imageIndex) {
    int i;
    int logLevel = TRACE;
    diskProperties->track0SectorCount = 0;
    diskProperties->track0SectorSize  = 0;
    diskProperties->track1SectorCount = 0;
    diskProperties->track1SectorSize  = 0;
    diskProperties->sideCount  = 0;
    diskProperties->trackCount = 0;
    
    for (i = 0; i < II_SECTORMAX; i++) {
       logger(logLevel, "%3d: tr/sd/sc/fl %2d/%2d/%2d/%02X at %05X.\n", i, imageIndex[i].track, imageIndex[i].side, 
                    imageIndex[i].sector, imageIndex[i].flags, imageIndex[i].sectorLocation);
        if (imageIndex[i].track == 255 && imageIndex[i].sector == 255) break;
        if (imageIndex[i].track == 0) diskProperties->track0SectorCount++;
        if (imageIndex[i].track == 1) diskProperties->track1SectorCount++;
        if ((imageIndex[i].side  + 1) > diskProperties->sideCount)  diskProperties->sideCount  = imageIndex[i].side  + 1;
        if ((imageIndex[i].track + 1) > diskProperties->trackCount) diskProperties->trackCount = imageIndex[i].track + 1;
        if (imageIndex[i].track == 0 && imageIndex[i].sector == 0 && imageIndex[i].side == 0)
            diskProperties->track0SingleDensity = (imageIndex[i].flags & SD_DOUBLE_DENSITY) ? 0 : 1;
        if (imageIndex[i].track == 1 && imageIndex[i].sector == 0 && imageIndex[i].side == 0)
            diskProperties->track1SingleDensity = (imageIndex[i].flags & SD_DOUBLE_DENSITY) ? 0 : 1; 
    }
    logger(INFO, "Image has %d tracks, %d sides.\n", diskProperties->trackCount, diskProperties->sideCount);
    logger(INFO, "Track 0: %d sectors, %s density.\n", 
            diskProperties->track0SectorCount, (diskProperties->track0SingleDensity) ? "Single" : "Double");
    logger(INFO, "Track 1: %d sectors, %s density.\n", 
            diskProperties->track1SectorCount, (diskProperties->track1SingleDensity) ? "Single" : "Double");
}

// Parses the imageIndex trying to find the requested sector based on track, side, sector.
// Fills in the sector location and flags in sectorParams if found
int getSectorLocation(SectorDescriptor_t *tSectorParams, SectorDescriptor_t *imageIndex) {
    int i;
    tSectorParams->sectorLocation = 0;
    for (i = 0; i < II_SECTORMAX; i++) {
        if (tSectorParams->track == imageIndex[i].track && 
                tSectorParams->sector == imageIndex[i].sector && tSectorParams->side == imageIndex[i].side) {
            logger(TRACE, "Found tr/sd/sc %d/%d/%d at %05X, index:%d\n", imageIndex[i].track, imageIndex[i].side, 
                    imageIndex[i].sector, imageIndex[i].sectorLocation, i);
            tSectorParams->sectorLocation = imageIndex[i].sectorLocation;
            tSectorParams->flags          = imageIndex[i].flags;
            return 1;
        }
    }
    return 0; // sector not found
}

int getSectorCountTrackSide(SectorDescriptor_t *tSectorParams, SectorDescriptor_t *imageIndex) {
    int i;
    int sectorCount = 0;
    for (i = 0; i < II_SECTORMAX; i++) {
        if (tSectorParams->track == imageIndex[i].track && tSectorParams->side == imageIndex[i].side) {
            sectorCount++;
        }
    }
    return sectorCount;
}

