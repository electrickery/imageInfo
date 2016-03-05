/* 
 * File:   teleDisk.h
 * Author: fjkraan
 *
 * Created on November 27, 2012, 7:14 PM
 */

#ifndef TELEDISK_H
#define	TELEDISK_H

#ifdef	__cplusplus
extern "C" {
#endif

//        Image header                (12 bytes)
//        ;If the image was created using "Advanced Compression", everything
//        ;below this line is compressed with LZSS-Huffman encoding.
//        Optional comment header     (10 bytes)
//        Optional comment data       (Variable size)
//          ;For each track on the disk ...
//          Track header              (4 bytes)
//            ;For each sector within the track
//            Sector header           (6 bytes)
//            Optional data header    (3 bytes)
//            Optional data block     (variable size)
//        ;Image ends with Trackheader beginning with 255 (FF hex)
//
//           
//                Image header:
//        Signature                   (2 bytes)
//        Sequence                    (1 byte)
//        Checksequence               (1 byte)
//        Teledisk version            (1 byte)
//        Data rate                   (1 byte)
//        Drive type                  (1 byte)
//        Stepping                    (1 byte)
//        DOS allocation flag         (1 byte)
//        Sides                       (1 byte)
//        Cyclic Redundancy Check     (2 bytes)
   
// http://www.fpns.net/willy/wteledsk.htm    
//struct file_head {
//char sig[3];  // signature "TD", nul terminated string
//BYTE  unkwn,  // doesn't seem to be used?
//      ver,    // for v2.11 through v2.16 seems to be 0x15
//      data1,  // 0x00 (except for akai)
//      type,   // see bios drive type 1-4,
//      flag,   // among other things can indicate comment field
//      data2,  // often 0x00
//      sides;  // 0x01 (always with rx50 or 0x02 with most MSDOS disks)
//WORD  crc;  //   crc of 1st 0xA bytes in record
//};

#ifdef	__cplusplus
}
#endif

#endif	/* TELEDISK_H */

