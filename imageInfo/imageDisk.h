/* 
 * File:   imageDisk.h
 * Author: fjkraan
 *
 * Created on November 28, 2012, 5:04 PM
 */

#ifndef IMAGEDISK_H
#define	IMAGEDISK_H

#ifdef	__cplusplus
extern "C" {
#endif

//         IMD v.vv: dd/mm/yyyy hh:mm:ss          (ASCII Header)
//         Comment (ASCII only - unlimited size)  (NOTE:  You can TYPE a .IMD)
//         1A byte - ASCII EOF character          (file to see header/comment)
    
#define IMD_HEADER_COMMENT_TERMINATOR 0x1A
    
//         - For each track on the disk:
//            1 byte  Mode value                  (0-5)
//            1 byte  Cylinder                    (0-n)
//            1 byte  Head                        (0-1)   (see Note)
//            1 byte  number of sectors in track  (1-n)
//            1 byte  sector size                 (0-6)
//            sector numbering map                * number of sectors
//            sector cylinder map (optional)      * number of sectors
//            sector head map     (optional)      * number of sectors
//            sector data records                 * number of sectors
    
typedef struct {
    unsigned char modeValue;
    unsigned char cylinder;
    unsigned char head;
    unsigned char sectorsInTrack;
    unsigned char sectorSize;
    unsigned char sectorNumberingMap[sectorsInTrack];
//    unsigned char cylinderMap[cylinder];
//    unsigned char headMap[head];
    unsigned char dataRecords[cylinder * head * sectorsInTrack * 128<<sectorSize];
} imd_trackHeader_t;   

//       6.1 Mode value
//
//          This value indicates the data transfer rate and density  in  which
//          the original track was recorded:
//
//             00 = 500 kbps FM   \   Note:   kbps indicates transfer rate,
//             01 = 300 kbps FM    >          not the data rate, which is
//             02 = 250 kbps FM   /           1/2 for FM encoding.
//             03 = 500 kbps MFM
//             04 = 300 kbps MFM
//             05 = 250 kbps MFM

#define IMD_MODE_500KBPS_FM  0 // 3 1/2" HD Single Density
#define IMD_MODE_300KBPS_FM  1 // 8" Single Density (and 5 1/4" HD Single Density)
#define IMD_MODE_250KBPS_FM  2 // 5 1/4" & 3 1/4" Single Density
#define IMD_MODE_500KBPS_MFM 3 // 3 1/2" High Density 
#define IMD_MODE_300KBPS_MFM 4 // 8" Double Density (and 5 1/4" HD Double Density)
#define IMD_MODE_250KBPS_MFM 5 // 5 1/4" & 3 1/4" Double Density
//
//       6.2 Sector size
//
//          The Sector Size value indicates the actual size of the sector data
//          occuring on the track:
//
//            00 =  128 bytes/sector
//            01 =  256 bytes/sector
//            02 =  512 bytes/sector
//            03 = 1024 bytes/sector
//            04 = 2048 bytes/sector
//            05 = 4096 bytes/sector
//            06 = 8192 bytes/sector

#define IMD_128_BYTE_SECTOR  0
#define IMD_256_BYTE_SECTOR  1
#define IMD_512_BYTE_SECTOR  2
#define IMD_1024_BYTE_SECTOR 3
#define IMD_2048_BYTE_SECTOR 4
#define IMD_4096_BYTE_SECTOR 5
#define IMD_8192_BYTE_SECTOR 6
//
//          ImageDisk does not currently handle disks with  differently  sized
//          sectors within the same  track  (the  PC  FDC  cannot  write  such
//          disks),  however an extension to the .IMD  file  format  has  been
//          suggested to allow these type of disks to be represented:
//
//             A sector size value of 0xFF indicates that a  table  of  sector
//             sizes occurs  after  the  sector  numbering/cylinder/head  maps
//             (immediately before the data records) which contains one 16-bit
//             value  (in little endian format)  per sector which defines  the
//             actual size of that sector.
//    ImageDisk                                                        Page: 27
//
//
//       6.3 Head value
//
//          This value indicates the side of the  disk  on  which  this  track
//          occurs (0 or 1).
//
//          Since HEAD can only be 0 or 1,  ImageDisk uses the upper  bits  of
//          this byte to indicate the presense of optional items in the  track
//          data:
//
//             Bit 7 (0x80) = Sector Cylinder Map
//             Bit 6 (0x40) = Sector Head     Map
//
#define IMD_TRACK_HEADER_HEAD_MASK      0x01
#define IMD_SECTOR_CYLINDER_MAP_PRESENT 0x80
#define IMD_SECTOR_HEAD_MAP_PRESENT     0x40


#ifdef	__cplusplus
}
#endif

#endif	/* IMAGEDISK_H */

