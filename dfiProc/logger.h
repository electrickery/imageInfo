/* 
 * File:   logger.h
 * Author: fjkraan
 *
 * Created on April 9, 2012, 5:39 PM
 */

#ifndef LOGGER_H
#define	LOGGER_H


#define LOUT_QUIET 0
#define LOUT_SUMMARY 1
#define LOUT_TSUMMARY 2
#define LOUT_SSUMMARY 3
#define LOUT_DEBUG 4
#define LOUT_ERRORS 5
#define LOUT_IDS 6
#define LOUT_HEX 7
#define LOUT_RAW 8
#define LOUT_SAMPLES 9

char *out_file_name;
FILE* out_file;


void setLogLevel(int level);

void setFileLogLevel(int level);

void logger(int level, const char *fmt, ...);



#endif	/* LOGGER_H */