/* 
 * File:   logger.h
 * Author: fjkraan
 *
 * Created on April 9, 2012, 5:39 PM
 */

#ifndef LOGGER_H
#define	LOGGER_H

#define ERROR 0
#define WARN  1
#define INFO  2
#define DEBUG 3
#define TRACE 4
#define BYTES 5

char *out_file_name;
FILE* out_file;

void setLogLevel(int level);

void logger(int level, const char *fmt, ...);

#endif	/* LOGGER_H */

