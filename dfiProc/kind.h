#ifndef KIND_H
#define	KIND_H


typedef struct {
   char* description;
  int cwclock;
  int fmthresh;   /* <= this value: short */
  int mfmthresh1; /* <= this value: short */
  int mfmthresh2; /* <= this value: short or medium */
  float mfmshort; /* nominal MFM short assuming 1x clock multiplier */
  int hd;         /* value for HD line when writing */
  int readtime;   /* read time in ms assuming RPM and clock are precise */
} kind_desc;

/* Note: kind numbers are used in usage message and on command line */
#define NKINDS 8

kind_desc getKindDesc(int kind);

#endif // KIND_H