
#include "kind.h"



static kind_desc kinds[NKINDS] = {
    
// Catweasel 
    
  /* 360 RPM, MFM data 300 kHz */
  { "5.25\" SD/DD disk in 1.2MB drive, Catweasel interface",
    2 /*14.161MHz*/, 70, 56, 79,
    23.600, 0, 167 },

  /* 300 RPM, MFM data 250 kHz */
  { "5.25\" SD/DD disk in 360KB/720KB drive, or 3.5\" SD/DD disk, Catweasel interface",
    2 /*14.161MHz*/, 84, 70, 92,
    28.322, 0, 200 },

  /* 360 RPM, MFM data 500 kHz */
  { "5.25\" HD disk, or 8\" SD/DD disk, Catweasel interface",
    2 /*14.161MHz*/, 42, 35, 46,
    14.161, 1, 167 },

  /* 300 RPM, MFM data 500 kHz */
  { "3.5\" HD disk, Catweasel interface", 
    2 /*14.161MHz*/, 42, 35, 46,
    14.161, 1, 200 },
    
//DiscFerret

  /* 360 RPM, MFM data 300 kHz */
  { "5.25\" SD/DD disk in 1.2MB drive, DiscFerret interface",
    4 /*25MHz*/, 123, 98, 139,
    25, 0, 167 },

  /* 300 RPM, MFM data 250 kHz */
  { "5.25\" SD/DD disk in 360KB/720KB drive, or 3.5\" SD/DD disk, DiscFerret interface",
    4 /*25MHz*/, 148, 123, 162,
    25, 0, 200 },

  /* 360 RPM, MFM data 500 kHz */
  { "5.25\" HD disk, or 8\" SD/DD disk, DiscFerret interface",
    4 /*25MHz*/, 74, 61, 81,
    25.0, 1, 167 },

  /* 300 RPM, MFM data 500 kHz */
  { "3.5\" HD disk, DiscFerret interface", 
    4 /*25MHz*/, 74, 61, 81,
    25.0, 1, 200 },};

kind_desc getKindDesc(int kind) {
    return kinds[kind];
}