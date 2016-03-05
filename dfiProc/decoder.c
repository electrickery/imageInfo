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
#include "decoder.h"
#include "kind.h"
#include "crc.h"
#include "logger.h"

int mfm_valid_clock(unsigned long long accum)
{
  /* Check for valid clock bits */
  unsigned int xclock = ~((accum >> 1) | (accum << 1)) & 0xaaaa;
  unsigned int clock = accum & 0xaaaa;
  if (xclock != clock) {
    logger(LOUT_ERRORS, "[clock exp %04x got %04x]", xclock, clock);
    return 0;
  }
  return 1;
}

/*
 * Convert Catweasel samples to strings of alternating clock/data bits
 * and pass them to process_bit for further decoding.
 * Ad hoc method using two fixed thresholds modified by a postcomp
 * factor.
 */
void process_sample(int sample, track_t *ttrack, userParameters_t *tuserParameters)
{
    static float adj = 0.0;
    int len;
    int cwclock    =   1;       // cw clock multiplier 1=7.0805 MHz, 2=14.161MHz, 4=28.322MHz
                                // df uses 25, 50, 100MHz
/*
    int fmthresh   = 123;
    int mfmthresh1 =  98;
    int mfmthresh2 = 139;
*/
    int fmthresh   = getKindDesc(tuserParameters->kind).fmthresh;
    int mfmthresh1 = getKindDesc(tuserParameters->kind).mfmthresh1;
    int mfmthresh2 = getKindDesc(tuserParameters->kind).mfmthresh2;
    
    float mfmshort = -1.0;
//    float postcomp = 0.5; // original
//    float postcomp = 0.1; // works with FM fmthresh   = 123;
    float postcomp = 0.0;   //   works with FM and MFM, effectively nulling adj

    logger(LOUT_SAMPLES, "%d", sample);
    if (tuserParameters->uencoding == FM) {
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

    logger(LOUT_SAMPLES, "%c ", "--sml"[len]);
//    logger(LOUT_SAMPLES, "%c(%0.2f) ", "--sml"[len], adj);

    process_bit(1, ttrack, 0);
    while (--len) process_bit(0, ttrack, 0);
}



void process_bit(int bit, track_t *ttrack, int uencoding) {
    //  static int curcyl = 0;
    unsigned char val = 0;
    int i;

    /* static vars. Set once, reset by hackerish argument values > 1 */
    static unsigned long long accum = 0;
    static unsigned long long taccum = 0;
    static int bits = 0;
    static unsigned short crc;
    static int mark_after = -1;
    static int write_splice = 0;
    static int curenc = 0;
    static int ibyte, dbyte;
    static int sizecode = 0;
    static unsigned char premark = 0;
    static int first_encoding; /* first encoding to try on next track */
    static int errcount;
    static int good_sectors = 0;
    static int enc_count[N_ENCS];
    static int total_enc_count[N_ENCS];
    static int valid_id = 0;
    static int awaiting_dam = 0;
    static int awaiting_iam = 0;
    static int sectorCount = 0;

    /* Hacks to re-initialize static variables. process_bit() is only used with argument values 0 and 1 */
    if (bit == 2) { /* Executed at the start of each track */
        /* process_bit local */
        accum = 0;
        taccum = 0;
        bits = 0;
        premark = 0;
        mark_after = -1;
        curenc = first_encoding;
        errcount = 0;
        for (i = 0; i < N_ENCS; i++) enc_count[i] = 0;
        sectorCount = 0;
        return;
    }
    if (bit == 3) { /* Executed at that start of each sector */
        ibyte = dbyte = -1; // set both ibyte as dbyte in inactive mode
        awaiting_dam = 0;
        valid_id = 0;
        errcount++;
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
     * achieve proper clock/data separation and proper byte-alignment.
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
            case 0x8aa2a2a88ULL: /* 0xfc / 0xd7: Index address mark. */
                //            8aa2a2a88: 100010101010001010100010101010001000 iam 0xfc
                //            88aa8aa8a: 100010001010101010001010101010001010 data 0xfc
            case 0x8aa222aa8ULL: /* 0xfe / 0xc7: ID address mark */
            case 0x8aa222888ULL: /* 0xf8 / 0xc7: Standard deleted DAM */
            case 0x8aa22288aULL: /* 0xf9 / 0xc7: RX02 deleted DAM / WD1771 user DAM */
            case 0x8aa2228a8ULL: /* 0xfa / 0xc7: WD1771 user DAM */
            case 0x8aa2228aaULL: /* 0xfb / 0xc7: Standard DAM */
            case 0x8aa222a8aULL: /* 0xfd / 0xc7: RX02 DAM */
                curenc = change_enc(curenc, FM);
                ttrack->sector[sectorCount].encoding = curenc;
                if (bits < 64 && bits >= 48) {
                    logger(LOUT_HEX, "(+%d)", 64 - bits);
                    bits = 64; /* byte-align by repeating some bits */
                } else if (bits < 48 && bits > 32) {
                    logger(LOUT_HEX, "(-%d)", bits - 32);
                    bits = 32; /* byte-align by dropping some bits */
                }
                mark_after = 32;
                premark = 0; // doesn't apply to FM marks
                break;

            case 0xa222a8888ULL: /* Backward 0xf8-0xfd DAM */
                curenc = change_enc(curenc, FM);
                ttrack->sector[sectorCount].encoding = curenc;
                backward_am++;
                logger(LOUT_ERRORS, "[backward AM] ");
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
                ttrack->sector[sectorCount].encoding = curenc;
                premark = 0xc2;
                if (bits < 64 && bits > 48) {
                    logger(LOUT_HEX, "(+%d)", 64 - bits);
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
                ttrack->sector[sectorCount].encoding = curenc;
                premark = 0xa1;
                if (bits < 64 && bits > 48) {
                    logger(LOUT_HEX, "(+%d)", 64 - bits);
                    bits = 64; /* byte-align by repeating some bits */
                }
                mark_after = bits;
                break;

            case 0x55555555:
                if (curenc == MFM && mark_after < 0 &&
                        ibyte == -1 && dbyte == -1 && !(bits & 1)) {
                    /* ff ff in gap.  This should probably be 00 00, so drop 1/2 bit */
                    logger(LOUT_HEX, "(-1)");
                    bits--;
                }
                break;

            case 0x92549254:
                if (mark_after < 0 && ibyte == -1 && dbyte == -1) {
                    /* 4e 4e in gap.  This should probably be byte-aligned */
                    curenc = change_enc(curenc, MFM);
                    ttrack->sector[sectorCount].encoding = curenc;
                    if (bits < 64 && bits > 48) {
                        /* Byte-align by dropping bits */
                        logger(LOUT_HEX, "(-%d)", bits - 48);
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
                    logger(LOUT_HEX, "(-%d)", i);
                    if (bits < 64) return;
                    break;
                }
            }
            if (i > 3) {
                /* Note bad clock pattern */
                logger(LOUT_HEX, "?");
            }
        }
        for (i = 0; i < 8; i++) {
            val |= (accum & (1ULL << (4 * i + 1 + 32))) >> (3 * i + 1 + 32);
        }
        bits = 32;

    } else if (curenc == MFM) {
        for (i = 0; i < 8; i++) {
            val |= (accum & (1ULL << (2 * i + 48))) >> (i + 48);
        }
        bits = 48;

    } else /* curenc == RX02 */ {
        for (i = 0; i < 8; i++) {
            val |= (taccum & (1ULL << (2 * i + 48))) >> (i + 48);
        }
        bits = 48;
    }

    if (mark_after == 0) {
        mark_after = -1;
        switch (val) {
            case 0xfc:
                /* Index address mark */
                if (curenc == MFM && premark != 0xc2) break;
                check_missing_dam(awaiting_dam);
                logger(LOUT_IDS, "\n#fc ");
                //      dmk_iam(0xfc, curenc);
                ibyte = -1;
                dbyte = -1;
                ttrack->indexMark = val;
                return;

            case 0xfe:
                /* ID address mark */
                if (curenc == MFM && premark != 0xa1) break;
                if (awaiting_iam) break;
                check_missing_dam(awaiting_dam);
                logger(LOUT_IDS, "\n#fe ");
                //      dmk_idam(0xfe, curenc);
                ibyte = 0;
                dbyte = -1;
                crc = calc_crc((curenc == MFM) ? 0xcdb4 : 0xffff, val);
                ttrack->sector[sectorCount].idam = val;
                return;

            case 0xf8: /* Standard deleted data address mark */
            case 0xf9: /* WD1771 user or RX02 deleted data address mark */
            case 0xfa: /* WD1771 user data address mark */
            case 0xfb: /* Standard data address mark */
            case 0xfd: /* RX02 data address mark */
                if (!awaiting_dam) {
                    logger(LOUT_ERRORS, "[unexpected DAM] ");
                    errcount++;
                    break;
                }
                awaiting_dam = 0;
                logger(LOUT_HEX, "\n");
                logger(LOUT_IDS, "\n#%2x ", val);
                //      dmk_data(val, curenc);
                if ((uencoding == MIXED || uencoding == RX02) &&
                        (val == 0xfd ||
                        (val == 0xf9 && (total_enc_count[RX02] + enc_count[RX02] > 0 ||
                        uencoding == RX02)))) {
                    curenc = change_enc(curenc, RX02);
                    ttrack->sector[sectorCount].encoding = curenc;
                }
                /* For MFM, premark a1a1a1 is included in the CRC */
                crc = calc_crc((curenc == MFM) ? 0xcdb4 : 0xffff, val);
                ibyte = -1; // switch from IDAM zone
                dbyte = secsize(sizecode, curenc) + 2; //  to DAM zone
                ttrack->sector[sectorCount].dam = val;
                return;

            case 0x80: /* MFM DAM or IDAM premark read backward */
                if (curenc != MFM || premark != 0xc2) break;
                backward_am++;
                logger(LOUT_ERRORS, "[backward AM] ");
                break;

            default:
                /* Premark with no mark */
                //logger(LOUT_ERRORS, "[dangling premark] ");
                //errcount++;
                break;
        }
    }

    switch (ibyte) {
        default:
            break;
        case 0:
            logger(LOUT_IDS, "cyl=");
            //    curcyl = val;
            ttrack->sector[sectorCount].track = val;
            break;
        case 1:
            logger(LOUT_IDS, "side=");
            ttrack->sector[sectorCount].head = val;
            break;
        case 2:
            logger(LOUT_IDS, "sec=");
            ttrack->sectorOrder[sectorCount] = val;
            ttrack->sector[sectorCount].sector = val;
            break;
        case 3:
            logger(LOUT_IDS, "size=");
            sizecode = val;
            ttrack->sector[sectorCount].sectorSize = val;
            break;
        case 4:
            logger(LOUT_HEX, "crc=");
            ttrack->sector[sectorCount].idam_crc1 = val;
            break;
        case 5:
            ttrack->sector[sectorCount].idam_crc2 = val;
            break;
        case 6:
            if (crc == 0) {
                logger(LOUT_IDS, "[good ID CRC] ");
                valid_id = 1;
            } else {
                logger(LOUT_ERRORS, "[bad ID CRC] ");
                errcount++;
                ibyte = -1;
            }
            logger(LOUT_HEX, "\n");
            awaiting_dam = 1;
            //    dmk_check_wraparound();
            break;
        case 18:
            /* Done with post-ID gap */
            logger(LOUT_IDS, "\n");
            ibyte = -1;
            break;
    }

    if (ibyte == 2) { // Sector number
        logger(LOUT_ERRORS, "%02x ", val);
    } else if (ibyte >= 0 && ibyte <= 3) { // Track number, Side number, Encoding
        logger(LOUT_IDS, "%02x ", val);
    } else { // 
        logger(LOUT_SAMPLES, "<");
        logger(LOUT_HEX, "%02x", val);
        logger(LOUT_SAMPLES, ">");
        logger(LOUT_HEX, " ", val);
        logger(LOUT_RAW, "%c", val);
    }

    //  dmk_data(val, curenc);
    if (dbyte > 2) { // 
        ttrack->sector[sectorCount].sectorData[secsize(sizecode, curenc) - dbyte + 2] = val;
    }
    if (dbyte == 2) {
        ttrack->sector[sectorCount].data_crc1 = val;
    }
    if (dbyte == 1) { // 0 is not reached, so 1 is last...
        ttrack->sector[sectorCount].data_crc2 = val;
    }

    if (ibyte >= 0) ibyte++;
    if (dbyte > 0) dbyte--;
    crc = calc_crc(crc, val);

    if (dbyte == 0) {
        if (crc == 0) {
            logger(LOUT_IDS, "[good data CRC]\n");
            if (valid_id) {
                if (good_sectors == 0) first_encoding = curenc;
                good_sectors++;
                enc_count[curenc]++;
                //	cylseen = curcyl;
            }
        } else {
            logger(LOUT_ERRORS, "[bad data CRC]\n");
            errcount++;
        }
        logger(LOUT_HEX, "\n");
        dbyte = -1;
        valid_id = 0;
        write_splice = WRITE_SPLICE;
        if (curenc == RX02) {
            curenc = change_enc(curenc, FM);
            ttrack->sector[sectorCount].encoding = curenc;
        }
        sectorCount++;
    }

    /* Predetect bad MFM clock pattern.  Can't detect at decode time
       because we need to look at 17 bits. */
    if (curenc == MFM && bits == 48 && !mfm_valid_clock(accum >> 32)) {
        if (mfm_valid_clock(accum >> 31)) {
            logger(LOUT_HEX, "(-1)");
            bits--;
        } else {
            logger(LOUT_HEX, "?");
        }
    }
}

void check_missing_dam(int dmk_awaiting_dam)
{
  if (!dmk_awaiting_dam) return;
  process_bit(3, NULL, 0);
  logger(LOUT_ERRORS, "[missing DAM] ");
}

int secsize(int sizecode, int encoding)
{
//    printf("sizecode: %d, encoding: %d\n", sizecode, encoding);
  int maxsize = 3;  /* 177x/179x look at only low-order 2 bits */
  switch (encoding) {
      case MFM:
        /* 179x can only do sizes 128, 256, 512, 1024, and ignores
           higher-order bits.  If you need to read a 765-formatted disk
           with larger sectors, change maxsize with the -z
           command line option. */
        return 128 << (sizecode % (maxsize + 1));
        break;
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
        break;
      case RX02:
        return 256 << (sizecode % (maxsize + 1));
        break;
      }
        
}

int change_enc(int curenc, int newenc)
{
  char *enc_name[] = { "autodetect", "FM", "MFM", "RX02" };
  if (curenc != newenc) {
    logger(LOUT_ERRORS, "[%s->%s] ", enc_name[curenc], enc_name[newenc]);
    return(newenc);
  }
  return(curenc);
}

