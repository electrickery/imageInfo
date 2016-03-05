/* 
 * File:   newdos.h
 * Author: fjkraan
 *
 * Created on December 22, 2012, 8:51 PM
 */

#ifndef NEWDOS_H
#define	NEWDOS_H


typedef struct {
    unsigned char DDSL_0;    // 00
    unsigned char lumps;     // 01
    unsigned char TI_TSR_2;  // 02
    unsigned char TC_3;      // 03
    unsigned char SPT_4;     // 04
    unsigned char GPL_5;     // 05
    unsigned char unknown_6; // 06
    unsigned char TI_TD_7;   // 07
    unsigned char DDSL_8;    // 08
    unsigned char DDGA_9;    // 09
    unsigned char unknown_A; // 0A
    char SPG_B;              // 0B (ND/86_90)
    unsigned char TSR_C;     // 0C
    unsigned char TI_D;      // 0D
    unsigned char TI_E;      // 0E
    unsigned char TD_F;      // 0F
} Pdrive_t;

#define TI_2_INTERFACE_MASK  0b00011100
#define TI_D_INTERFACE_MASK  0b00011111

#define TI_2_OPTION_MASK     0b11100000
#define TI_7_OPTION_MASK     0b00010110
#define TI_2_H_MASK          0b10000000 // 02-7
#define TI_7_I_MASK          0b00000100 // 07-2
#define TI_7_J_MASK          0b00000010 // 07-1
#define TI_2_K_MASK          0b01000000 // 02-6
#define TI_7_L_MASK          0b00000100 // 07-2
#define TI_2_M_MASK          0b00100000 // 02-5

#define TD_7_MASK                0b11000001
#define TD_7_DOUBLE_DENSITY_MASK 0b00000001
#define TD_7_DOUBLE_SIDED_MASK   0b01000000
#define TD_7_8INCH_MASK          0b10000000
#define TD_F_MASK                0b00000111
#define TD_F_DOUBLE_DENSITY_MASK 0b00000100
#define TD_F_DOUBLE_SIDED_MASK   0b00000010
#define TD_F_8INCH_MASK          0b00000001

#define TSR_2C_MASK              0b00000011


//00   01    02 03 04  05  06 07 08   09   0a  0b 0c  0d 0e 0f
//DDSL Lumps *  TC SPT GPL 8" *  DDSL DDGA        TSR TI TI TD

//00 - DDSL
//01 - Lumps
//02 - TSR (bits 0,1)
//     TI (A = bit 2
//         B = bit 2&3
//         C = bit 4
//         D = bit -
//         E = bit 2&4
//         M = bit 5
//         K = bit 6
//         H = bit 7)
//03 - TC
//04 - SPT
//05 - GPL (bits 3 - 0 = values 2 - 8)
//         (NEWDOS/86 and /90 use the upper nibble for the SPG
//          Offset = 5: 2=D, 3=E, 4=F, 5=0, 6=1, 7=2, ...)
//06 - 5.25" (80h) or 8" (C0h) drive select command (at 47C0h) [TI=E]
//           (FCh)       (FDh)                                 [TI=B]
//07 - TD (bit 0: DD (TD=E,F,G,H)
//         bit 6: DS (=C,D,G,H)
//         bit 7: 8-inch (=B, D, F, H))
//     TI (bit 1: J,K
//         bit 2: L
//         bit 4: I)
//08 - DDSL
//09 - DDGA
//0A - always 00
//0B - always 00 (NEWDOS/86 and /90 use it for the SPG)
//0C - TSR
//0D - TI (bit 0: A,
//         bit 1: B,
//         bit 2: C,
//         bit 3: D,
//         bit 4: E,
//         bit 7: H)
//0E - TI (bit 0: I
//         bit 1: J
//         bit 2: K
//         bit 3: L
//         bit 4: M)
//0F - TD (bit 0: 8-inch
//         bit 1: DS
//         bit 2: DD)

int logPDRIVE(int logLevel, unsigned char *data);
int pdriveTDmatcher(unsigned char td7, unsigned char tdF);
int pDriveTIDmatcher(unsigned char ti2, unsigned char tiD);

#endif	/* NEWDOS_H */



