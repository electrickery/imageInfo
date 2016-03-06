/*
 *	Vfloppy image structure
 */

/*
 * UU F1 F2 F3 F4 F5 F6 F7 F8 T1 T2 T3 EX S1 S2 RC   .FILENAMETYP....
 * AL AL AL AL AL AL AL AL AL AL AL AL AL AL AL AL   ................
 */

/* generic directory information, offset from start of image */
#define DIRBASE		0x80
#define DIRINCR		0x20
#define DIRENTRYSIZE	0x20
#define DIRENTRYCOUNT	0x40

#define BLOCKBASE	0x0080
#define BLOCKSIZE	0x0800
#define RECSIZE		0x80
#define RECSPERBLOCK	0x10
#define MAXRECCOUNT	0x80
#define MAXBLOCKCOUNT	256

#define DISKSIZE	0x8b 	/* (139 blocks, 278 kByte) */
 
/* directory entry information, offset from start of dir. entry*/
#define DIRUSERNO	0   	/* UU byte		*/
#define DIRNAMEBASE	1   	/* Fx bytes		*/
#define DIREXTBASE	9	/* Tx bytes		*/
#define DIRNAMESIZE	11  	/* Fx + Tx byte count	*/
#define DIRENTRYNOFIELD	12  	/* EX byte bit 0: 0=last entry, 1=intermediate
				 *         bits 1-7 used for count */
#define DIREXTCNTHIGH	13	/* S1 byte extra directory entry count bits */
#define DIRS2		14	/* S2 byte		*/
#define DIRRECS		15	/* RC byte		*/
/* AL bytes */
#define DIRBLOCKBASE	16  	/* first AL byte	*/
#define DIRBLOCKSIZE	16  	/* AL byte count	*/

#define EXM		0x1f	/* Extend mask		*/
#define FMTPATT		0xe5	/* disk formatting pattern
				   also used to delete directory entry */
#define DEBUG		0


extern char *fileName;
extern char dirFileName[];
extern char *imageName;
extern int fnd;
extern int imd;
extern unsigned char blockList[];
extern unsigned char data[];
extern int lastRecCnt;
extern int entryNo;
extern int totalUsedBlockCnt;
extern int errno;
extern int fileSize;
extern int fileBlks;
extern int fileDirEntries;
extern int debug;

extern void showDirEntry(unsigned char *dent);
extern int blockRead(int fd, unsigned char *data, unsigned long blockStart, int len);
extern int blockWrite(int fd, unsigned char *data, unsigned long block, int len);
extern void showDir(int id);
extern int mountImage(char *imName);
extern int mountImageRW(char *imName);
extern void convertFileName(unsigned char *dFName, unsigned char *flName);
extern int cmpEntry(unsigned char *dent, char *dFlName);
extern void updateBlockList(unsigned char *dent);
extern int findFile(int id, unsigned char *dFName);
extern int createFile(unsigned char *fileName);
extern int checkFileNSize(char *fn);
extern int cntUsedImgDirEntries(int id, char *dFn);
extern int cntUsedImgBlocks(int id, char *dFn);
extern int countBlocks(char *blkList);
extern void eraFile(int id, char *dFName);
extern void getFreeBlockList(int id);
extern int writeDirEntry(int id, char *dirEnt);
extern void writeDir(int id, char *dFn, char *fn);
extern int writeBlocks(char *fileName);
