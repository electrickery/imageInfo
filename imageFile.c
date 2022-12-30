/* 
 * File:   imageFile.c
 * Author: fjkraan
 *
 * Created on March 28, 2014, 1:28 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "imageFile.h"
/*
 * 
 */

int getImageSize(userParameters_t* tuserParameters) {
    fseek(tuserParameters->fileHandle, 0, SEEK_END);
    return ftell(tuserParameters->fileHandle);
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

int readFromImage(void *data, long location, int size, userParameters_t *tuserParameters) {
    int result;
    logger(TRACE, "Reading %d bytes at location %05X\n", size, location);
    
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
    
    return 1;
}

void dataUnDoubler(unsigned char *doubledData, unsigned char *singledData, int size, int interval) {
    int i;
    if (interval < 1) interval = 1;
    if (interval > 2) interval = 2;
    int range = size / interval;
    int offset = interval - 1;
    
    for (i = 0; i < range; i++) {
        singledData[i] = doubledData[i * interval + offset];
//        logger(BYTES, "%02X ", singledData[i]);
    }
//    logger(BYTES, "\n");  
    logBinaryBlock(BYTES, singledData, range);
}



