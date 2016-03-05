/* 
 * File:   dfiProcess.c version 0.3
 * Author: fjkraan
 * 
 * Based on decoding routines Copyright (C) 2000 Timothy Mann 
 * Might depend on Linux Catweasel driver code by Michael Krause
 *
 * Created on January 21, 2012, 2:50 PM
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <math.h>
#include <signal.h>
#if linux
#include <sys/io.h>
#endif
#include "dataStructs.h"
#include "dfiProcess.h"
#include "kind.h"
#include "crc.h"
#include "logger.h"
#include "decoder.h"

int fd_img; 
struct trackHeader trackHeader;

void openImage(char *dImage) {
    fd_img = open(dImage, O_RDONLY);
    if (fd_img == -1) {
        logger(LOUT_ERRORS, "Some error with opening %s. Exiting...\n", dImage);
        perror(dImage);
        exit(2);
    }
    logger(LOUT_DEBUG, "Image file: \'%s\', (fd: %d) opened ok.\n", dImage, fd_img);
}

int checkImageHeader(void) {
    int err;
    int headerSize = 4;
    char data[headerSize];
    char* header = "DFE2";
    
    lseek(fd_img, 0, 0);
    
    err = read(fd_img, data, headerSize);
    if (err < 0) {
        logger(LOUT_ERRORS, "  checkImageHeader; err=%d\n", err);
        return err;
    }
    if (err < headerSize) {
        logger(LOUT_ERRORS, "  checkImageHeader; file too short: %d bytes\n", err);
    }
    if (strcmp(data, header) != 0) {
        logger(LOUT_ERRORS, "  checkImageHeader; file header mismatch '%s'\n", data);
        return(0);
    }
    logger(LOUT_DEBUG, "  checkImageHeader; file header ok.\n");
    return(1);
}

unsigned int processTrackHeader(unsigned int pos, track_t *ttrack) {
    int trackHeaderSize = 10; // sizeof(trackHeader); the reported size is 12 bytes, not 10!
    
    unsigned char data[trackHeaderSize];
    int err;
    
    lseek(fd_img, pos, 0);
    
    err = read(fd_img, data, trackHeaderSize);
    if (err < 0) {
        logger(LOUT_ERRORS, "  processTrackHeader; err=%d\n", err);
        return err;
    }
    if (err == 0) {
        logger(LOUT_DEBUG, "  processTrackHeader; End of file\n");
        exit(0);
    }
    if (err < trackHeaderSize) {
        logger(LOUT_ERRORS, "  processTrackHeader; file too short: %d bytes\n", err);
        return(0);
    }
    trackHeader.track     = (data[0] << 8) + data[1];
    trackHeader.head      = (data[2] << 8) + data[3];
    trackHeader.sector    = (data[4] << 8) + data[5];
    trackHeader.trackSize = (data[6] << 24) + (data[7] << 16) + (data[8] << 8) + data[9];
    
    ttrack->hardwareTrack = trackHeader.track;
    ttrack->hardwareSide  = trackHeader.head;
    
    logger(LOUT_DEBUG, "  processTrackHeader; track: %hd, head: %hd, sector: %hd, trackSize: %08X\n", 
            trackHeader.track, 
            trackHeader.head,
            trackHeader.sector,
            trackHeader.trackSize);
    return (pos + trackHeaderSize);
}

void initHistArray(histoGraph_t *hist) {
    int i;
    for (i = 0; i < hist->histSize; i++) {
        hist->hist[i] = 0;
    }
}

void trackSummary(track_t *ttrack) {
    int i = 0;
    int j;
//    int k;
//    int size;
//    logger(LOUT_TSUMMARY, " trackSummary; ");
    logger(LOUT_TSUMMARY, " trackSummary; ");
    logger(LOUT_TSUMMARY, " track: %d", ttrack->hardwareTrack);
    logger(LOUT_TSUMMARY, " side: %d", ttrack->hardwareSide);
    while (ttrack->sectorOrder[i] != -1 && i < MAXSECTORSIZE) { i++; }
    logger(LOUT_TSUMMARY, " sectors: %d", i);
    i = 0;
    logger(LOUT_TSUMMARY, " order: ");
    while (ttrack->sectorOrder[i] != -1 && i < MAXSECTORSIZE) {
        logger(LOUT_TSUMMARY, "%d, ", ttrack->sectorOrder[i]);
        i++;
    }
    logger(LOUT_TSUMMARY, "\n");
    for (j = 0; j < i; j++) { // i points to the first not used sector
        logger(LOUT_SSUMMARY, "  sector: size=%d idam=%02x cyl=%d side=%d sec=%d enc=%d icrc=%02x %02x",
//            j, 
            ttrack->sector[j].sectorSize,
            ttrack->sector[j].idam,
            ttrack->sector[j].track,
            ttrack->sector[j].head,
            ttrack->sector[j].sector,
            ttrack->sector[j].encoding,
            ttrack->sector[j].idam_crc1,
            ttrack->sector[j].idam_crc2);
        logger(LOUT_SSUMMARY, " dam=%02x crc=%02x %02x\n", 
            ttrack->sector[j].dam,
            ttrack->sector[j].data_crc1,
            ttrack->sector[j].data_crc2);
/*
        size = secsize(track->sector[j].sectorSize, track->sector[j].encoding);
        for (k = 0; k < size; k++) {
            printf("%02x ", track->sector[j].sectorData[k]);
        }
        printf("\n");
*/
    }
}

int findIndex(int *sectorOrder, int value, int max) {
    int i;
    for (i = 0; i < max; i++) {
        if (sectorOrder[i] == value) return i;
    }
    return -1;
}

FILE* openDumpFile(userParameters_t *tuserParameters) {
    FILE *file = fopen(tuserParameters->imageFile, "wb");
    if (file == NULL) {
      logger(LOUT_ERRORS, "  openDumpFile; error opening %s\n", tuserParameters->imageFile);
    }    
    return file;
}

void closeDumpFile(FILE *file) {
    fclose(file);
}

void jv1Dump(track_t *ttrack, userParameters_t *tuserParameters) {
    int i = 0;
    int j;
    int size;
    logger(LOUT_DEBUG, "Dumping to %s\n", tuserParameters->imageFile);
    i = 0;
    while (i < MAXTRACKSECTORS) {
        j = findIndex(ttrack->sectorOrder, i, MAXTRACKSECTORS);
        if (j != -1) {
            logger(LOUT_DEBUG, "Found sector %d at %d, ", i, j);
            size = secsize(ttrack->sector[j].sectorSize, ttrack->sector[j].encoding);
            logger(LOUT_DEBUG, "dumping %d bytes\n", size);
            fwrite(&ttrack->sector[j].sectorData, size, 1, tuserParameters->fileHandle);
        }
        i++;
    }
}

int getSample (int dfiValue) {                 // either the carry tally is returned or -1 in case of an overflow value
    static int carry = 0;
    int tmpCarry;
    if (dfiValue == -1) {
        carry = 0;
        return 0;
    }
    if (dfiValue == DFI_OVERFLOW) {             // overflow value, add to carry
        carry += dfiValue;
        return -1;
    } else if (dfiValue > DFI_OVERFLOW) {       // index hole bit set
        logger(LOUT_DEBUG, "  processTrack; index found!\n");
        if (dfiValue == 0xFF) {                 // overflow & index hole bit, add to carry
            carry += DFI_OVERFLOW;
            return -1;
        } else {                                // non-overflow & index hole bit
            carry += dfiValue & DFI_OVERFLOW;
        }
    } else {                                    // non-overflow value
        carry += dfiValue;
    }
    tmpCarry = carry;
    carry = 0;
    return tmpCarry;

}

int getLowest(histoGraph_t *histoGraph) {
    int i;
    int lowest = 0x7FFFFFFF; // assumed max int
    for (i=0; i < histoGraph->histSize; i++) {
        lowest = (histoGraph->hist[i] < lowest) ? histoGraph->hist[i] : lowest;
    }
    return lowest;
}

int getHighest(histoGraph_t *histoGraph) {
    int i;
    int highest = -1; // lower enough 
    for (i=0; i < histoGraph->histSize; i++) {
        highest = (histoGraph->hist[i] > highest) ? histoGraph->hist[i] : highest;
    }
    return highest;
}

int fillHistoGraph(histoGraph_t *histoGraph, char *dfiData, int trackSize) {
    int i;
    int samples = 0;
    int carry = 0;
    for (i = 0; i < trackHeader.trackSize - 1; i++) {
         carry = getSample(dfiData[i]);
         if (carry != -1) {
            histoGraph->hist[carry & (histoGraph->histSize - 1)] += 1;
            samples++;
        }
    }    
    return samples;
}

unsigned int processTrack(unsigned int pos, int silent, 
        track_t *ttrack, userParameters_t *tuserParameters) {
    if (!silent) {
        int i, carry, err, l1_samples;
//        int lowest, highest;
        
        process_bit(2, NULL, 0); // Hack to re-initialize 
        process_bit(3, NULL, 0);
        
        histoGraph_t hs;                   // make an instance
        histoGraph_t *histoGraph = &hs;    // pointerize it
        histoGraph->histSize = HISTSIZE;
        initHistArray(histoGraph);
        
        int tresHoldFactor = 200;
        char dfiData[trackHeader.trackSize];
        lseek(fd_img, pos, 0);

        err = read(fd_img, dfiData, trackHeader.trackSize);
        if (err < 0) {
            logger(LOUT_ERRORS, "  processTrackHeader; err=%d\n", err);
            return err;
        }
        if (err < trackHeader.trackSize) {
            logger(LOUT_ERRORS, "  processTrack; file too short: %d bytes\n", err);
            return(0);
        }
        
        l1_samples = fillHistoGraph(histoGraph, dfiData, trackHeader.trackSize);
        printHist(histoGraph, (l1_samples/tresHoldFactor));
        logger(LOUT_DEBUG, "Lowest value: %d, Highest value: %d\n", getLowest(histoGraph), getHighest(histoGraph));

        carry = 0;
//        lowest = 0xFFFFFF;
//        highest = 0;
        for (i = 0; i < trackHeader.trackSize - 1; i++) {
            carry = getSample(dfiData[i]);
            if (carry != -1) {
                process_sample(carry, ttrack, tuserParameters);
                carry = 0;
            }
            
    //        msg(OUT_DEBUG, "%02X, ", data[i]);
        }
//        msg(OUT_DEBUG, "\nValues sampled: %d, lowest: %0X, highest: %0X\n", printed, lowest, highest);

        logger(LOUT_DEBUG, "  processTrack; adding trackSize: %08X to pos: %08X\n", trackHeader.trackSize, pos);
        
        trackSummary(ttrack);
        if (tuserParameters->imageType != -1) jv1Dump(ttrack, tuserParameters);
    } else {
        logger(LOUT_DEBUG, "  processTrack; skipping track\n");
    }
    return(pos + trackHeader.trackSize);
}

void printHist(histoGraph_t *hist, int printedTresHoldFactor) {
        int peakCount = 0;
        int inPeak = 0;
        int i;
        logger(LOUT_DEBUG, "Histogram:\n");
        for (i = 0; i < hist->histSize; i++) {
            if (hist->hist[i] > (printedTresHoldFactor)) {
                logger(LOUT_DEBUG, "%0X:%d,  ", i, hist->hist[i]);
                if (inPeak == 0) {
                    peakCount++;
                    inPeak = 1;
                } 
            } else {
                if (inPeak ==1)
                    logger(LOUT_DEBUG, "|\n");

                inPeak = 0;
            }
        }   
        logger(LOUT_DEBUG, "Peaks: %d\n", peakCount);
}

void usage(userParameters_t *tuserParameters) {
    printf(" usage; -d <dfiImageFile> : (mandatory)\n");
    printf("        -e encoding   1 = FM (SD), 2 = MFM (DD or HD), 3 = RX02\n");
    printf("        -f file name\n");
    printf("        -i file type. Only 1, JV1 is supported\n");
    printf("        -k   1 = %s\n", getKindDesc(0).description);
    printf("             2 = %s\n", getKindDesc(1).description);
    printf("             3 = %s\n", getKindDesc(2).description);
    printf("             4 = %s\n", getKindDesc(3).description);
    printf("             5 = %s\n", getKindDesc(4).description);
    printf("             6 = %s\n", getKindDesc(5).description);
    printf("             7 = %s\n", getKindDesc(6).description);
    printf("             8 = %s\n", getKindDesc(7).description);
    printf("        -s : skip odd tracks in image\n");  
    printf("        -v verbosity  Amount of output [%d]\n", tuserParameters->out_level);
    printf("               0 = No output\n");
    printf("               1 = Summary of disk\n");
    printf("               2 = + summary of each track\n");
    printf("               3 = + individual errors\n");
    printf("               4 = + track IDs and DAMs\n");
    printf("               5 = + hex data and event flags\n");
    printf("               6 = like 4, but with raw data too\n");
    printf("               7 = like 5, but with Catweasel samples too\n");
    printf("               21 = level 2 to logfile, 1 to screen, etc.\n");
}

void scrubTrackBuffer(track_t *ttrack) {
    int i,j;
    ttrack->hardwareTrack = -1;
    ttrack->hardwareSide = -1;
    ttrack->indexMark = -1;   
    for (i = 0; i < MAXTRACKSECTORS; i++) {
        ttrack->sectorOrder[i] = -1;
        ttrack->sector[i].encoding = -1,
        ttrack->sector[i].dam = -1;
        ttrack->sector[i].data_crc1 = -1;
        ttrack->sector[i].data_crc2 = -1;
        ttrack->sector[i].head = -1;
        ttrack->sector[i].idam = -1;
        ttrack->sector[i].idam_crc1 = -1;
        ttrack->sector[i].idam_crc2 = -1;
        ttrack->sector[i].number = -1;
        ttrack->sector[i].sector = -1;
        ttrack->sector[i].sectorSize = -1;
        ttrack->sector[i].track = -1;
        for (j = 0; j < MAXSECTORSIZE; j++) {
            ttrack->sector[i].sectorData[j] = 0;
        }
    }
}

void optionParse(int argc, char** argv, userParameters_t *tuserParameters) {
    char *optstr = "d:e:f:i:k:shv:";
    char ch;
     while( -1 != (ch=getopt(argc,argv,optstr))) {
        switch(ch) {
            case 'd':           
                openImage(optarg);
                break;
            case 'e':
                tuserParameters->uencoding = strtol(optarg, NULL, 0);
                if (tuserParameters->uencoding < FM || tuserParameters->uencoding > RX02) {
                    printf("Encoding not valid: %d\n", tuserParameters->uencoding);
                    usage(tuserParameters);
                    exit(1);
                }
                break;
            case 'f':
                tuserParameters->imageFile = optarg;
                break;                
            case 'h':
                usage(tuserParameters);
                exit(1);
                break;
            case 'i':
                tuserParameters->imageType = strtol(optarg, NULL, 0);
                break;
            case 'k':
                tuserParameters->kind = strtol(optarg, NULL, 0);
                if (tuserParameters->kind < 1 || tuserParameters->kind > 8) {
                    usage(tuserParameters);
                    exit(1);
                }
                break;
            case 's':
                tuserParameters->skipTracks = 1;
                break;
            case 'v':
                tuserParameters->out_level = strtol(optarg, NULL, 0);
                if (tuserParameters->out_level < LOUT_QUIET || tuserParameters->out_level > LOUT_SAMPLES * 11) {
                  usage(tuserParameters);
                  exit(1);
                  break;
                }
                tuserParameters->out_file_level = tuserParameters->out_level / 10;
                tuserParameters->out_level = tuserParameters->out_level % 10;
                break;
            case '?':
            default:
                printf(" main; error?  condition unaccounted for?\n");
                usage(tuserParameters);
                exit(1);
                break;
        }
    }   
}

int main(int argc, char** argv) {
   int trackCount = 0;
    int silentTrack;
    int pos = 4; /* fileHeaderSize */
//    int skipOddTracks = 0;
    
    userParameters_t ups;
    userParameters_t *userParameters = &ups;

    userParameters->uencoding = FM;
    userParameters->skipTracks = 0;
    userParameters->out_level = 0;
    userParameters->out_level = LOUT_QUIET;
    userParameters->out_file_level = LOUT_QUIET;
    userParameters->imageType = -1;
    
    track_t tt;
    track_t *track = &tt;

    optionParse(argc, argv, userParameters);
    if (userParameters->kind == 0) {
                usage(userParameters);
                exit(1);
    }
    setLogLevel(userParameters->out_level);

    if (!checkImageHeader()) {
        return(EXIT_FAILURE);
    }

    userParameters->fileHandle = openDumpFile(userParameters);
    if (userParameters->fileHandle == 0) {
        logger(LOUT_ERRORS, "Error opening %s, exiting\n", userParameters->imageFile);
        exit(1);
    }
    while(1) {
        scrubTrackBuffer(track);
        pos = processTrackHeader(pos, track);
        if (pos == 0) return(EXIT_SUCCESS);
        logger(LOUT_DEBUG, " main; returned track position: %08X\n", pos);

        silentTrack = userParameters->skipTracks && isOdd(trackCount);
        pos = processTrack(pos, silentTrack, track, userParameters);
        if (pos == 0) return(EXIT_SUCCESS);
        logger(LOUT_DEBUG, " main; returned next track header position: %08X\n", pos);
        logger(LOUT_DEBUG, "------------- next track -------------\n");
        trackCount++;
        if (trackCount > 159) break;
    }
    closeDumpFile(userParameters->fileHandle);
    return (EXIT_SUCCESS);
}

int isOdd(int value) {
//    logger(LOUT_DEBUG, "  isOdd; returning %d for %d\n", value % 2, value);
    return (value % 2);
}




char* plu(int val)
{
  return (val == 1) ? "" : "s";
}

/*
void check_missing_dam(int dmk_awaiting_dam)
{
  if (!dmk_awaiting_dam) return;
  process_bit(3);
  logger(LOUT_ERRORS, "[missing DAM] ");
}
*/

