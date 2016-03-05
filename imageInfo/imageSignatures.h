/* 
 * File:   imageSignatures.h
 * Author: fjkraan
 *
 * Created on December 13, 2012, 7:06 PM
 */

#ifndef IMAGESIGNATURES_H
#define	IMAGESIGNATURES_H

#ifdef	__cplusplus
extern "C" {
#endif

#define BOOTSECTORSIGNATURE_TERMINATOR  0xFFFF
#define BOOTSECTORSIGNATURE_IGNORE      0xFF00
#define BOOTSECTORSIGNATURE_DIR_TRACK   0xFF01
    
// Attempt to create an out of band values for "ignore value" (0xFF00) and "end of array" (0xFFFF)
// Added new value: "directory track" (0xFF01)
unsigned short bootSectorSignature_trsdos23[] = { 0x00, 0xFE, 0xFF01, 0xFFFF };
unsigned short bootSectorSignature_trsdos27[] = { 0xFE, 0xFF01, 0x3E, 0xFFFF }; // FE 11 3E FF 32 EC 37
unsigned short bootSectorSignature_trsdos13[] = { 0x00, 0xFE, 0xFF01, 0xF3,   0xFFFF };
unsigned short bootSectorSignature_dosplus[]  = { 0xF3, 0xFE, 0xFF01, 0xFFFF };
unsigned short bootSectorSignature_loboLdos[] = { 0x00, 0x1E, 0xFF01, 0xFFFF };
unsigned short bootSectorSignature_trsdos6[]  = { };
    
#ifdef	__cplusplus
}
#endif

#endif	/* IMAGESIGNATURES_H */

