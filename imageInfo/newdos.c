/* 
 * File:   newdos.c
 * Author: fjkraan
 *
 * Created on March 23, 2014, 2:44 PM
 */

#include <stdio.h>
#include <string.h>


#include "logger.h"
#include "newdos.h"

unsigned char trackStepRate[] = { 5, 10, 20, 40 };

const char *pdriveInterfaces[] = {
    "undefined 0",                             //
    "A - Standard Tandy Interface",            // 02-2
    "D - Percom doubler Interface (Mod I)",    //       02-3
    "B - Apparat Disk Controller  (Mod III)",  // 02-2, 02-3
    "C - Omikron mapper Interface (Mod I)",    //             02-4
    "E - LNW type Interface (Mod I)",          // 02-2,       02-4
    "undefined 06",                            //       02-3, 02-4
    "undefined 07"};                           // 02-2, 02-3, 02-4

int logPDRIVE(int logLevel, unsigned char *data) {
    int validPdrive0Entry = 1;
    Pdrive_t pdrive0;
    int pdTI;
    int lowerLogLevel = logLevel + 1;
    
//    logBinaryBlock(INFO, data, 256);
    memcpy(&pdrive0, &data[0], sizeof(Pdrive_t));
    
    // internal consistency checks
    if (pdrive0.TI_TSR_2 == 0) {
        logger(lowerLogLevel, "** Pdrive error flag set **\n");
        validPdrive0Entry = 0;
    }
    if (!pdriveTDmatcher(pdrive0.TI_TD_7, pdrive0.TD_F)) {
        logger(lowerLogLevel, "Pdrive TD fields do not match\n");
        validPdrive0Entry = 0;
    }
    if (pdrive0.TC_3 < 34 || pdrive0.TC_3 > 83) {
        logger(lowerLogLevel, "Pdrive TC value out of range: %d\n", pdrive0.TC_3);
        validPdrive0Entry = 0;
    }
    if (!pDriveTIDmatcher(pdrive0.TI_TSR_2, pdrive0.TI_D)) {
        logger(lowerLogLevel, "Pdrive TI values don't match\n");
        validPdrive0Entry = 0;
    }
    pdTI = (pdrive0.TI_TSR_2 & TI_2_INTERFACE_MASK)>>2;
    if (pdTI == 0 || pdTI > 5) {
        logger(lowerLogLevel, "Pdrive TI value invalid: %d\n", pdTI);
        validPdrive0Entry = 0;
    }
    if ((pdrive0.TI_TSR_2 & TSR_2C_MASK) != (pdrive0.TSR_C & TSR_2C_MASK)) {
        logger(lowerLogLevel, "Pdrive TSR fields do not match: %02X %02X\n", 
                pdrive0.TI_TSR_2 & TSR_2C_MASK, pdrive0.TSR_C & TSR_2C_MASK);
        validPdrive0Entry = 0;
    }
    if (!validPdrive0Entry) {
        logger(logLevel, "PDRIVE drive 0 entry not valid\n");
        return 0;
    }
    logger(logLevel, "PDRIVE drive 0:\n");
    logger(logLevel, "TI:   %s ", pdriveInterfaces[pdTI]);
    logger(logLevel + 1, " (%02X %02X)", (pdrive0.TI_TSR_2 & TI_2_INTERFACE_MASK), (pdrive0.TI_D & TI_D_INTERFACE_MASK));
    logger(logLevel, "\n");
    logger(logLevel, "%s", (pdrive0.TI_TSR_2 & TI_2_H_MASK) ? "      H - Head settle delay enabled (8 inch Drives)\n" : "");
    logger(logLevel, "%s", (pdrive0.TI_TD_7  & TI_7_I_MASK) ? "      I - Sector 1 = lowest Sector on each Track\n" : "");
    logger(logLevel, "%s", (pdrive0.TI_TD_7  & TI_7_J_MASK) ? "      J - Track 1 = lowest Track on the Diskette\n" : "");
    logger(logLevel, "%s", (pdrive0.TI_TSR_2 & TI_2_K_MASK) ? "      K - Track 0 formatted in opposite density to the rest\n" : "");
    logger(logLevel, "%s", (pdrive0.TI_TD_7  & TI_7_L_MASK) ? "      L - 2 steps between tracks (read 40Trk on 80Trk drive)\n" : "");
    logger(logLevel, "%s", (pdrive0.TI_TSR_2 & TI_2_M_MASK) ? "      M - TRSDOS ModI 2.3B or higher or ModIII read\n" : "");
    logger(logLevel, "TD:   %s Disk, ",    (pdrive0.TI_TD_7 & TD_7_8INCH_MASK) ? "8\"" : "5 1/4\"");
    logger(logLevel, "%s Density, ", (pdrive0.TI_TD_7 & TD_7_DOUBLE_DENSITY_MASK) ? "Double" : "Single");
    logger(logLevel, "%s Sided ",  (pdrive0.TI_TD_7 & TD_7_DOUBLE_SIDED_MASK) ? "Double" : "Single");
    logger(logLevel + 1, "(%02X %02X)", (pdrive0.TI_TD_7),  (pdrive0.TD_F));
    logger(logLevel, "\n");
    logger(logLevel, "TC:   %2d tracks\n", (pdrive0.TC_3));
    logger(logLevel, "SPT:  %2d sectors per track\n", (pdrive0.SPT_4));
    logger(logLevel, "TSR:  %02d ms track step rate", trackStepRate[pdrive0.TI_TSR_2 & TSR_2C_MASK]);
    logger(logLevel + 1, " (%02X %02X)", (pdrive0.TI_TSR_2 & TSR_2C_MASK), (pdrive0.TSR_C & TSR_2C_MASK));
    logger(logLevel, "\n");
    logger(logLevel, "Lmps: %d lumps on disk\n", pdrive0.lumps);
    logger(logLevel, "GPL:  %d granules per lump\n", (pdrive0.GPL_5));
    logger(logLevel, "DDSL: %d; Disk Directory Starting Lump", (pdrive0.DDSL_0));
    logger(logLevel + 1, " (%2d)", (pdrive0.DDSL_8));
    logger(logLevel, "\n");
    logger(logLevel, "DDGA: %d; Number of Granules allocated to Directory\n", (pdrive0.DDGA_9));
    logger(logLevel, "SPG:  %d; Sectors per Granule (ND/86 and /90 only)\n", (pdrive0.SPG_B + 5));
//    if ((pdrive1.TI_TSR & TI_2_INTERFACE_MASK) == 0) logger(logLevel, " %02X\n", pdrive0.TI_TSR & TI_2_INTERFACE_MASK);
//    logger(logLevel, "\n");
    return 1;
}

int pdriveTDmatcher(unsigned char td7, unsigned char tdF) {
    unsigned char reshuffledF = 0;
    
    reshuffledF += (tdF & TD_F_DOUBLE_DENSITY_MASK) ? TD_7_DOUBLE_DENSITY_MASK : 0;
    reshuffledF += (tdF & TD_F_DOUBLE_SIDED_MASK)   ? TD_7_DOUBLE_SIDED_MASK   : 0;
    reshuffledF += (tdF & TD_F_8INCH_MASK)          ? TD_7_8INCH_MASK          : 0;
    logger(TRACE, "pdriveTDmatcher: %02X, %02X (%02X)\n", td7, tdF, reshuffledF);
    
    if (td7 == reshuffledF) return 1;
    return 0;
}

int pDriveTIDmatcher(unsigned char ti2, unsigned char tiD) {
    unsigned char ti2masked = ti2 & TI_2_INTERFACE_MASK;
    unsigned char tiDmasked = tiD & TI_D_INTERFACE_MASK;
    
    logger(TRACE, "pdriveTImatcher: %02X, %02X\n", ti2masked, tiDmasked);
    if (ti2masked == 0x04 && tiDmasked == 0x01) return 1; // TI=A
    if (ti2masked == 0x14 && tiDmasked == 0x02) return 1; //    B
    if (ti2masked == 0x0C && tiDmasked == 0x04) return 1; //    C 
    if (ti2masked == 0x08 && tiDmasked == 0x08) return 1; //    D
    if (ti2masked == 0x14 && tiDmasked == 0x10) return 1; //    E
    
    return 0;
}

