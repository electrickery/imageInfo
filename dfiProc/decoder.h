/* 
 * File:   decoder.h
 * Author: fjkraan
 *
 * Created on February 26, 2012, 4:13 PM
 */

#ifndef DECODER_H
#define	DECODER_H

/* Window used to undo RX02 MFM transform */
#if 1
#define WINDOW 4   /* change aligned 1000 -> 0101 */
#else
#define WINDOW 12  /* change aligned x01000100010 -> x00101010100 */
#endif

/* Suppress FM address mark detection for a few bit times after each
   data CRC is seen.  Helps prevent seening bogus marks in write
   splices. */
#define WRITE_SPLICE 32

#define DFI_OVERFLOW 0x7F



/* Constants and globals for decoding */
#define MIXED 0
#define FM 1
#define MFM 2
#define RX02 3
#define N_ENCS 4


/* Decoding routines Copyright (C) 2000 Timothy Mann */

int mfm_valid_clock(unsigned long long accum);

void process_bit(int bit, track_t *ttrack, int uencoding);

void process_sample(int sample, track_t *ttrack, userParameters_t *tuserParameters);

void check_missing_dam(int dmk_awaiting_dam);

int secsize(int sizecode, int encoding);

int change_enc(int curenc, int newenc);


#endif	/* DECODER_H */