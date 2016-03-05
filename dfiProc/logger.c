#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

static int out_level = 0;
static int out_file_level = 0;

void setLogLevel(int level) {
    out_level = level;
}

void setLogFileLevel(int level) {
    out_file_level = level;
}

void logger(int level, const char *fmt, ...)
{
  va_list args;

  if (level <= out_level &&
      !(level == LOUT_RAW && out_level != LOUT_RAW) &&
      !(level == LOUT_HEX && out_level == LOUT_RAW)) {
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
  }
  if (out_file && level <= out_file_level &&
      !(level == LOUT_RAW && out_file_level != LOUT_RAW) &&
      !(level == LOUT_HEX && out_file_level == LOUT_RAW)) {
    va_start(args, fmt);
    vfprintf(out_file, fmt, args);
    va_end(args);
  }
}
