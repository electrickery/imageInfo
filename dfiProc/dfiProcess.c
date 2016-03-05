/* 
 * File:   main.c
 * Author: fjkraan
 *
 * Created on January 21, 2012, 2:50 PM
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
#include "dfiProcess.h"
//#include "dmk.h"

int fd_img; 
struct trackHeader trackHeader;
int pos; /* fileHeaderSize */
int skipOddTracks;
int uencoding;
//int curenc;
unsigned char premark;

unsigned long long accum, taccum;
int bits;

int mark_after;
int write_splice; /* bit counter, >0 if we may be in a write splice */
int first_encoding;  /* first encoding to try on next track */

char *enc_name[] = { "autodetect", "FM", "MFM", "RX02" };

  int good_sectors;
  int total_good_sectors;
  int total_errcount;
  int errcount;
  int enc_count[N_ENCS];
  int total_enc_count[N_ENCS]; 
  int err_tracks;
  int good_tracks;
  
int index_edge;
int out_level = OUT_HEX;
int out_file_level = OUT_QUIET;
int dmk_iam_pos = -1;
int dmk_ignore = 0;
int cylseen = -1;
unsigned short crc;

unsigned char* dmk_track = NULL;


/*
 * 
 */

void openImage(char *dImage) {
    fd_img = open(dImage, O_RDONLY);
    if (fd_img == -1) {
        printf("Some error with opening %s. Exiting...\n", dImage);
        perror(dImage);
        exit(2);
    }

    printf("Image file: \'%s\', (fd: %d) opened ok.\n", dImage, fd_img);
}

int checkImageHeader() {
    int err;
    int headerSize = 4;
    char data[headerSize];
    char* header = "DFE2";
    
    if(lseek(fd_img, 0, 0)==-1);
    
    err = read(fd_img, data, headerSize);
    if (err < 0) {
        printf("  checkImageHeader; err=%d\n", err);
        return err;
    }
    if (err < headerSize) {
        printf("  checkImageHeader; file too short: %d bytes\n", err);
    }
    if (strcmp(data, header) != 0) {
        printf("  checkImageHeader; file header mismatch '%s'\n", data);
        return(0);
    }
    printf("  checkImageHeader; file header ok.\n");
    return(1);
}

unsigned int processTrackHeader(unsigned int pos) {
    int trackHeaderSize = 10; // sizeof(trackHeader); the reported size is 12 bytes, not 10!
    
    unsigned char data[trackHeaderSize];
    int err;
    
    if(lseek(fd_img, pos, 0)==-1);
    
    err = read(fd_img, data, trackHeaderSize);
    if (err < 0) {
        printf("  processTrackHeader; err=%d\n", err);
        return err;
    }
    if (err < trackHeaderSize) {
        printf("  processTrackHeader; file too short: %d bytes\n", err);
        return(0);
    }
    trackHeader.track     = (data[0] << 8) + data[1];
    trackHeader.head      = (data[2] << 8) + data[3];
    trackHeader.sector    = (data[4] << 8) + data[5];
    trackHeader.trackSize = (data[6] << 24) + (data[7] << 16) + (data[8] << 8) + data[9];
    
    printf("  processTrackHeader; track: %hd, head: %hd, sector: %hd, trackSize: %08X\n", 
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

unsigned int processTrack(unsigned int pos, int silent) {
    if (!silent) {
        int i, carry, err, printed;
        int lowest, highest;
        
        process_bit(2); // Hack to re-initialize 
        process_bit(3);
        
        histoGraph_t hs;                   // make an instance
        histoGraph_t *histoGraph = &hs;    // pointerize it
        histoGraph->histSize = HISTSIZE;
        initHistArray(histoGraph);
        char overflow      = 0x7F;
        int tresHoldFactor = 200;
        char dfiData[trackHeader.trackSize];
        if(lseek(fd_img, pos, 0)==-1);

        err = read(fd_img, dfiData, trackHeader.trackSize);
        if (err < 0) {
            printf("  processTrackHeader; err=%d\n", err);
            return err;
        }
        if (err < trackHeader.trackSize) {
            printf("  processTrack; file too short: %d bytes\n", err);
            return(0);
        }

        carry = 0;
        printed = 0;
        lowest = 0xFFFFFF;
        highest = 0;
        for (i = 0; i < trackHeader.trackSize - 1; i++) {
            if (dfiData[i] == overflow) {
                carry += dfiData[i];
            } else if (dfiData[i] > overflow) {
                printf("  processTrack; index found!\n");
            } else {
                carry += dfiData[i];
    //            printf("%03X, ", carry);
                printed++;
                lowest = (carry  < lowest)  ? carry : lowest;
                highest = (carry > highest) ? carry : highest;
                histoGraph->hist[carry & (histoGraph->histSize - 1)] += 1;
                
                index_edge = 0;

                process_sample(carry);
                carry = 0;
            }
    //        printf("%02X, ", data[i]);
        }
        printf("\nValues printed: %d, lowest: %0X, highest: %0X\n", printed, lowest, highest);

        printHist(histoGraph, (printed/tresHoldFactor));
        
        printf("  processTrack; adding trackSize: %08X to pos: %08X\n", trackHeader.trackSize, pos);
    } else {
        printf("  processTrack; skipping track\n");
    }
    return(pos + trackHeader.trackSize);
}

void printHist(histoGraph_t *hist, int printedTresHoldFactor) {
        int peakCount = 0;
        int inPeak = 0;
        int i;
        for (i = 0; i < hist->histSize; i++) {
            if (hist->hist[i] > (printedTresHoldFactor)) {
                printf("%0X:%d,  ", i, hist->hist[i]);
                if (inPeak == 0) {
                    peakCount++;
                    inPeak = 1;
                } 
            } else {
                if (inPeak ==1)
                    printf("|\n");

                inPeak = 0;
            }
        }   
        printf("Peaks: %d\n", peakCount);
}

int main(int argc, char** argv) {
    char ch;
    char *optstr = "d:sh";
    int trackCount = 0;
    int silentTrack;
    pos = 4; /* fileHeaderSize */
    skipOddTracks = 0;
    uencoding = MIXED;
    index_edge = 0;

    while( -1 != (ch=getopt(argc,argv,optstr))) {
        switch(ch) {
            case 'd':           
                openImage(optarg);
                break;
            case 's':
                skipOddTracks = 1;
                break;
            case '?':
            case 'h':
                printf(" usage; -d <dfiImageFile> : (mandatory)\n");
                printf("        -s : skip odd tracks in image\n");
                exit(1);
                break;
            default:
                printf(" main; error?  condition unaccounted for?\n");
                break;
        }
    }

    if (!checkImageHeader()) {
        return(EXIT_FAILURE);
    }
    /* Command-line parameters */
//int fmtimes = 2; /* record FM bytes twice; see man page */

 //    dmk_track = dmk_init(trackHeader.track, DMK_TRACKLEN_5, trackHeader.head, fmtimes);
    
    while(1) {
        pos = processTrackHeader(pos);
        if (pos == 0) return(EXIT_SUCCESS);
        printf(" main; returned track position: %08X\n", pos);

        silentTrack = skipOddTracks && isOdd(trackCount);
        pos = processTrack(pos, silentTrack);
        if (pos == 0) return(EXIT_SUCCESS);
        printf(" main; returned track header position: %08X\n", pos);
        printf("------------- next track -------------\n");
        trackCount++;
  //      if (trackCount > 1) break;
    }

    return (EXIT_SUCCESS);
}

int isOdd(int value) {
    printf("  isOdd; returning %d for %d\n", value % 2, value);
    return (value % 2);
}

void process_bit(int bit) {
  static int curcyl = 0;
  unsigned char val = 0;
  int i;
  
  /* static vars */
   static unsigned long long accum = 0;
   static unsigned long long taccum = 0;
   static int bits = 0;
   static int mark_after = -1;
   static int write_splice = 0;
   static int curenc = 0;
   static int ibyte, dbyte;
   static int sizecode = 0;

  if (bit == 2) { /* Hack to re-initialize static variables. process_bit() is only used with argument values 0 and 1 */
      /* process_bit local */
      accum = 0;
      taccum = 0;
      bits = 0;
      premark = 0;
      mark_after = -1; 
      curenc = first_encoding;
      return;
  }
   if (bit == 3) {
      ibyte = dbyte = -1; // set both ibyte as dbyte in inactive mode
      return; 
  }
  
  /* interim localised vars */
//  unsigned short crc;
  int backward_am = 0;

  

  accum = (accum << 1) + bit;
  taccum = (taccum << 1) + bit;
  bits++;
  if (mark_after >= 0) mark_after--;
  if (write_splice > 0) write_splice--;

  /*
   * Pre-detect address marks: we shift bits into the low-order end of
   * our 64-bit shift register (accum), look for marks in the lower
   * half, but decode data from the upper half.  When we recognize a
   * mark (or certain other patterns), we repeat or drop some bits to
   * achieve proper clock/data separatation and proper byte-alignment.
   * Pre-detecting the marks lets us do this adjustment earlier and
   * decode data more cleanly.
   *
   * We always sample bits at the MFM rate (twice the FM rate), but
   * we look for both FM and MFM marks at the same time.  There is
   * ambiguity here if we're dealing with normal (not DEC-modified)
   * MFM, because FM marks can be legitimate MFM data.  So we don't
   * look for FM marks while we think we're inside an MFM ID or data
   * block, only in gaps.  With -e2, we don't look for FM marks at
   * all.
   */

  /*
   * For FM and RX02 marks, we look at 9 data bits (including a
   * leading 0), which ends up being 36 bits of accum (2x for clocks,
   * another 2x for the double sampling rate).  We must not look
   * inside a region that can contain standard MFM data.
   */
  if (uencoding != MFM && bits >= 36 && !write_splice &&
      (curenc != MFM || (ibyte == -1 && dbyte == -1 && mark_after == -1))) {
        switch (accum & 0xfffffffffULL) {
        case 0x8aa2a2a88ULL:  /* 0xfc / 0xd7: Index address mark */
        case 0x8aa222aa8ULL:  /* 0xfe / 0xc7: ID address mark */
        case 0x8aa222888ULL:  /* 0xf8 / 0xc7: Standard deleted DAM */
        case 0x8aa22288aULL:  /* 0xf9 / 0xc7: RX02 deleted DAM / WD1771 user DAM */
        case 0x8aa2228a8ULL:  /* 0xfa / 0xc7: WD1771 user DAM */
        case 0x8aa2228aaULL:  /* 0xfb / 0xc7: Standard DAM */
        case 0x8aa222a8aULL:  /* 0xfd / 0xc7: RX02 DAM */
          curenc = change_enc(curenc, FM);
          if (bits < 64 && bits >= 48) {
            msg(OUT_HEX, "(+%d)", 64-bits);
            bits = 64; /* byte-align by repeating some bits */
          } else if (bits < 48 && bits > 32) {
            msg(OUT_HEX, "(-%d)", bits-32);
            bits = 32; /* byte-align by dropping some bits */
          }
          mark_after = 32;
          premark = 0; // doesn't apply to FM marks
          break;

        case 0xa222a8888ULL:  /* Backward 0xf8-0xfd DAM */
          curenc = change_enc(curenc, FM);
          backward_am++;
          msg(OUT_ERRORS, "[backward AM] ");
          break;
        }
  }

  /*
   * For MFM premarks, we look at 16 data bits (two copies of the
   * premark), which ends up being 32 bits of accum (2x for clocks).
   */
  if (uencoding != FM && uencoding != RX02 &&
      bits >= 32 && !write_splice) {
    switch (accum & 0xffffffff) {
    case 0x52245224:
      /* Pre-index mark, 0xc2c2 with missing clock between bits 3 & 4
	 (using 0-origin big-endian counting!).  Would be 0x52a452a4
	 without missing clock. */
      curenc = change_enc(curenc, MFM);
      premark = 0xc2;
      if (bits < 64 && bits > 48) {
	msg(OUT_HEX, "(+%d)", 64-bits);
	bits = 64; /* byte-align by repeating some bits */
      }
      mark_after = bits;
      break;

    case 0x44894489:
      /* Pre-address mark, 0xa1a1 with missing clock between bits 4 & 5
	 (using 0-origin big-endian counting!).  Would be 0x44a944a9
	 without missing clock.  Reading a pre-address mark backward
	 also matches this pattern, but the following byte is then 0x80. */
      curenc = change_enc(curenc, MFM);
      premark = 0xa1;
      if (bits < 64 && bits > 48) {
	msg(OUT_HEX, "(+%d)", 64-bits);
	bits = 64; /* byte-align by repeating some bits */
      }
      mark_after = bits;
      break;

    case 0x55555555:
      if (curenc == MFM && mark_after < 0 &&
	  ibyte == -1 && dbyte == -1 && !(bits & 1)) {
	/* ff ff in gap.  This should probably be 00 00, so drop 1/2 bit */
	msg(OUT_HEX, "(-1)");
	bits--;
      }
      break;

    case 0x92549254:
      if (mark_after < 0 && ibyte == -1 && dbyte == -1) {
	/* 4e 4e in gap.  This should probably be byte-aligned */
	curenc = change_enc(curenc, MFM);
	if (bits < 64 && bits > 48) {
	  /* Byte-align by dropping bits */
	  msg(OUT_HEX, "(-%d)", bits-48);
	  bits = 48;
	}
      }
      break;
    }
  }

  /* Undo RX02 DEC-modified MFM transform (in taccum) */
#if WINDOW == 4
  if (bits >= WINDOW && (bits & 1) == 0 && (accum & 0xfULL) == 0x8ULL) {
    taccum = (taccum & ~0xfULL) | 0x5ULL;
  }
#else /* WINDOW == 12 */
  if (bits >= WINDOW && (bits & 1) == 0 && (accum & 0x7ffULL) == 0x222ULL) {
    taccum = (taccum & ~0x7ffULL) | 0x154ULL;
  }
#endif

  if (bits < 64) return;

  if (curenc == FM || curenc == MIXED) {
    /* Heuristic to detect being off by some number of bits */
    if (mark_after != 0 && ((accum >> 32) & 0xddddddddULL) != 0x88888888ULL) {
      for (i = 1; i <= 3; i++) {
	if (((accum >> (32 - i)) & 0xddddddddULL) == 0x88888888ULL) {
	  /* Ignore oldest i bits */
	  bits -= i;
	  msg(OUT_HEX, "(-%d)", i);
	  if (bits < 64) return;
	  break;
	}
      }
      if (i > 3) {
	/* Note bad clock pattern */
	msg(OUT_HEX, "?");
      }
    }
    for (i=0; i<8; i++) {
      val |= (accum & (1ULL << (4*i + 1 + 32))) >> (3*i + 1 + 32);
    }
    bits = 32;

  } else if (curenc == MFM) {
    for (i=0; i<8; i++) {
      val |= (accum & (1ULL << (2*i + 48))) >> (i + 48);
    }
    bits = 48;

  } else /* curenc == RX02 */ {
    for (i=0; i<8; i++) {
      val |= (taccum & (1ULL << (2*i + 48))) >> (i + 48);
    }
    bits = 48;
  }

  if (mark_after == 0) {
    mark_after = -1;
    switch (val) {
    case 0xfc:
      /* Index address mark */
      if (curenc == MFM && premark != 0xc2) break;
      check_missing_dam();
      msg(OUT_IDS, "\n#fc ");
//      dmk_iam(0xfc, curenc);
      ibyte = -1;
      dbyte = -1;
      return;

    case 0xfe:
      /* ID address mark */
      if (curenc == MFM && premark != 0xa1) break;
      if (dmk_awaiting_iam) break;
      check_missing_dam();
      msg(OUT_IDS, "\n#fe ");
//      dmk_idam(0xfe, curenc);
      ibyte = 0;
      crc = calc_crc1((curenc == MFM) ? 0xcdb4 : 0xffff, val);
      dbyte = -1;
      return;

    case 0xf8: /* Standard deleted data address mark */
    case 0xf9: /* WD1771 user or RX02 deleted data address mark */
    case 0xfa: /* WD1771 user data address mark */
    case 0xfb: /* Standard data address mark */
    case 0xfd: /* RX02 data address mark */
      if (!dmk_awaiting_dam) {
	msg(OUT_ERRORS, "[unexpected DAM] ");
	errcount++;
	break;
      }
      dmk_awaiting_dam = 0;
      msg(OUT_HEX, "\n");
      msg(OUT_IDS, "#%2x ", val);
//      dmk_data(val, curenc);
      if ((uencoding == MIXED || uencoding == RX02) &&
	  (val == 0xfd ||
	   (val == 0xf9 && (total_enc_count[RX02] + enc_count[RX02] > 0 ||
			    uencoding == RX02)))) {
	curenc = change_enc(curenc, RX02);
      }
      /* For MFM, premark a1a1a1 is included in the CRC */
      crc = calc_crc1((curenc == MFM) ? 0xcdb4 : 0xffff, val);
      ibyte = -1;                               // switch from IDAM zone
      dbyte = secsize(sizecode, curenc) + 2;    //  to DAM zone
      printf("dbyte: %X ", dbyte);
      return;

    case 0x80: /* MFM DAM or IDAM premark read backward */
      if (curenc != MFM || premark != 0xc2) break;
      backward_am++;
      msg(OUT_ERRORS, "[backward AM] ");
      break;

    default:
      /* Premark with no mark */
      //msg(OUT_ERRORS, "[dangling premark] ");
      //errcount++;
      break;
    }
  }

  switch (ibyte) {
  default:
    break;
  case 0:
    msg(OUT_IDS, "cyl=");
    curcyl = val;
    break;
  case 1:
    msg(OUT_IDS, "side=");
    break;
  case 2:
    msg(OUT_IDS, "sec=");
    break;
  case 3:
    msg(OUT_IDS, "size=");
    sizecode = val;
    break;
  case 4:
    msg(OUT_HEX, "crc=");
    break;
  case 6:
    if (crc == 0) {
      msg(OUT_IDS, "[good ID CRC] ");
      dmk_valid_id = 1;
    } else {
      msg(OUT_ERRORS, "[bad ID CRC] ");
      errcount++;
      ibyte = -1;
    }
    msg(OUT_HEX, "\n");
    dmk_awaiting_dam = 1;
//    dmk_check_wraparound();
    break;
  case 18:
    /* Done with post-ID gap */
    ibyte = -1;
    break;
  }

  if (ibyte == 2) {
    msg(OUT_ERRORS, "%02x ", val);
  } else if (ibyte >= 0 && ibyte <= 3) {
    msg(OUT_IDS, "%02x ", val);
  } else {
    msg(OUT_SAMPLES, "<");
    msg(OUT_HEX, "%02x", val);
    msg(OUT_SAMPLES, ">");
    msg(OUT_HEX, " ", val);
    msg(OUT_RAW, "%c", val);
  }

//  dmk_data(val, curenc);

  if (ibyte >= 0) ibyte++;
  if (dbyte > 0) dbyte--;
  crc = calc_crc1(crc, val);

  if (dbyte == 0) {
    if (crc == 0) {
      msg(OUT_IDS, "[good data CRC] ");
      if (dmk_valid_id) {
	if (good_sectors == 0) first_encoding = curenc;
	good_sectors++;
	enc_count[curenc]++;
	cylseen = curcyl;
      }
    } else {
      msg(OUT_ERRORS, "[bad data CRC] ");
      errcount++;
    }
    msg(OUT_HEX, "\n");
    dbyte = -1;
    dmk_valid_id = 0;
    write_splice = WRITE_SPLICE;
    if (curenc == RX02) {
      curenc = change_enc(curenc, FM);
    }
  }

  /* Predetect bad MFM clock pattern.  Can't detect at decode time
     because we need to look at 17 bits. */
  if (curenc == MFM && bits == 48 && !mfm_valid_clock(accum >> 32)) {
    if (mfm_valid_clock(accum >> 31)) {
      msg(OUT_HEX, "(-1)");
      bits--;
    } else {
      msg(OUT_HEX, "?");
    }
  }    
}

/*
 * Convert Catweasel samples to strings of alternating clock/data bits
 * and pass them to process_bit for further decoding.
 * Ad hoc method using two fixed thresholds modified by a postcomp
 * factor.
 */
void process_sample(int sample)
{
    static float adj = 0.0;
    int len;
    int cwclock    =   4;       // cw clock multiplier 1=7.0805 MHz, 2=14.161MHz, 4=28.322MHz
                                // df uses 25, 50, 100MHz
    int fmthresh   = 123;
    int mfmthresh1 =  98;
    int mfmthresh2 = 139;
    float mfmshort = -1.0;
    float postcomp = 0.1; // original: 0.5
    int uencoding = FM;

 //   msg(OUT_SAMPLES, "%d", sample);
    if (uencoding == FM) {
      if (sample + adj <= fmthresh) {
        /* Short */
        len = 2;
      } else {
        /* Long */
        len = 4;
      }
    } else {
      if (sample + adj <= mfmthresh1) {
        /* Short */
        len = 2;
      } else if (sample + adj <= mfmthresh2) {
        /* Medium */
        len = 3;
      } else {
        /* Long */
        len = 4;
      }
    }
    adj = (sample - (len/2.0 * mfmshort * cwclock)) * postcomp;

    msg(OUT_SAMPLES, "%c ", "--sml"[len]);
//    msg(OUT_SAMPLES, "%c(%0.2f) ", "--sml"[len], adj);

    process_bit(1);
    while (--len) process_bit(0);
}

/*
void init_decoder(void)
{
    // process_bit(2); re-initialize static local vars
  accum = 0;
  taccum = 0;
  bits = 0;
  ibyte = dbyte = -1;
  premark = 0;
  mark_after = -1;
//  curenc = first_encoding;
}
*/

int change_enc(int curenc, int newenc)
{
  if (curenc != newenc) {
    msg(OUT_ERRORS, "[%s->%s] ", enc_name[curenc], enc_name[newenc]);
    return(newenc);
  }
  return(curenc);
}

/* Log a message. */

char *out_file_name;
FILE* out_file;

void msg(int level, const char *fmt, ...)
{
  va_list args;

  if (level <= out_level &&
      !(level == OUT_RAW && out_level != OUT_RAW) &&
      !(level == OUT_HEX && out_level == OUT_RAW)) {
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
  }
  if (out_file && level <= out_file_level &&
      !(level == OUT_RAW && out_file_level != OUT_RAW) &&
      !(level == OUT_HEX && out_file_level == OUT_RAW)) {
    va_start(args, fmt);
    vfprintf(out_file, fmt, args);
    va_end(args);
  }
}


int secsize(int sizecode, int encoding)
{
    printf("sizecode: %d, encoding: %d\n", sizecode, encoding);
  int maxsize = 3;  /* 177x/179x look at only low-order 2 bits */
  switch (encoding) {
  case MFM:
    /* 179x can only do sizes 128, 256, 512, 1024, and ignores
       higher-order bits.  If you need to read a 765-formatted disk
       with larger sectors, change maxsize with the -z
       command line option. */
    return 128 << (sizecode % (maxsize + 1));

  case FM:
  default:
    /* WD1771 has two different encodings for sector size, depending on
       a bit in the read/write command that is not recorded on disk.
       We guess IBM encoding if the size is <= maxsize, non-IBM
       if larger.  This doesn't really matter for demodulating the
       data bytes, only for checking the CRC.  */
    if (sizecode <= maxsize) {
      /* IBM */
      return 128 << sizecode;
    } else {
      /* non-IBM */
      return 16 * (sizecode ? sizecode : 256);
    }

  case RX02:
    return 256 << (sizecode % (maxsize + 1));
  }
}

int mfm_valid_clock(unsigned long long accum)
{
  /* Check for valid clock bits */
  unsigned int xclock = ~((accum >> 1) | (accum << 1)) & 0xaaaa;
  unsigned int clock = accum & 0xaaaa;
  if (xclock != clock) {
    msg(OUT_ERRORS, "[clock exp %04x got %04x]", xclock, clock);
    return 0;
  }
  return 1;
}

char* plu(int val)
{
  return (val == 1) ? "" : "s";
}

/* crc.c
   Compute CCITT CRC-16 using the correct bit order for floppy disks.
   $Id: crc.c,v 1.2 2001/06/12 07:31:43 mann Exp $
*/


/* Slow way, not using table */
unsigned short CALC_CRC1a(unsigned short crc, unsigned char byte)
{
  int i = 8;
  unsigned short b = byte << 8;
  while (i--) {
    crc = (crc << 1) ^ (((crc ^ b) & 0x8000) ? 0x1021 : 0);
    b <<= 1;
  }
  return crc;
}



/* Recompute the CRC with len bytes appended. */
unsigned short calc_crc(unsigned short crc,
			unsigned char const *buf, int len) 
{
  while (len--) {
    crc = calc_crc1(crc, *buf++);
  }
  return crc;
}

void check_missing_dam(void)
{
  if (!dmk_awaiting_dam) return;
  dmk_awaiting_dam = 0;
  dmk_valid_id = 0;
  process_bit(3);
  errcount++;
  msg(OUT_ERRORS, "[missing DAM] ");
}
