/* 
 * File:   dfiProcess.h
 * Author: fjkraan
 *
 * Created on February 10, 2012, 7:36 PM
 */

#ifndef DFIPROCESS_H
#define	DFIPROCESS_H




/*
3  trackHi - track high byte
4  trackLo - track low byte
5  headHi  - head high byte
6  headLo  - head low byte
7  sectHi  - sector high byte
8  sectLo  - sector low byte
9  nbyte3  - number of bytes high byte
A  nbyte2  - number of bytes mid-high byte
B  nbyte1  - number of bytes mid-low byte
C  nbyte0  - number of bytes low byte 
 */

struct trackHeader {
    unsigned short track;
    unsigned short head;
    unsigned short sector;
    int trackSize;
};

#define HISTSIZE 512;

typedef struct  {
    int histSize;
    int hist[512];
} histoGraph_t;




#define HISTSIZE 512;


void openImage(char *dImage);

int checkImageHeader(void);

int isOdd(int value);

unsigned int processTrackHeader(unsigned int pos, track_t *ttrack);

unsigned int processTrack(unsigned int pos, int silent, 
        track_t *ttrack, userParameters_t *tuserParameters);

void initHistArray(histoGraph_t *hist);

void printHist(histoGraph_t *hist, int printedTresHoldFactor);





#endif	/* DFIPROCESS_H */