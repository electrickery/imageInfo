
#include <stdio.h>
#include <stdarg.h>

#include "logger.h"


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

