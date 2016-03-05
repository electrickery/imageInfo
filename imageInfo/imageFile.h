/* 
 * File:   imageFile.h
 * Author: fjkraan
 *
 * Created on March 28, 2014, 1:30 PM
 */

#ifndef IMAGEFILE_H
#define	IMAGEFILE_H

typedef struct {
    char *imageFile;
    FILE *fileHandle;
    int imageType;
    const char *errorMessage;
    int showDirectory;
    int forceSingleSided;
} userParameters_t ;


int getImageSize(userParameters_t* tuserParameters);

int openImage(userParameters_t* tuserParameters);

int readFromImage(void *data, long location, int size, userParameters_t *tuserParameters);
void dataUnDoubler(unsigned char *doubledData, unsigned char *singledData, int size, int interval);

void closeImage(userParameters_t* tuserParameters);
void shutdown(userParameters_t* tuserParameters);






#endif	/* IMAGEFILE_H */

