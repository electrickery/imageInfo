Yet another image interpreter

This program identifies disk images for TRS-80 Model I, III and 4 machines. Only
JV1, JV3 and DMK are currently supported. 

Maybe I'll directory reading later.

usage: -d display directory tracks
       -f <image file>
       -h              - help and version
       -s force single sided interpretation
       -v <log level>  - verbosity (0-4), default 1
                         0 - ERROR; 1 - WARN; 2 - INFO; 3 - DEBUG; 4 - TRACE; 5 - BYTES 5
       -?              - help and version

New in 0.7:
- implemented technical GAT, HIT and directory entry listings (-d option). Just for 
  the basic SD, SS TRSDOS 2.3 compatible disks.
- implemented a boot sector signature check, which also extracts the directory track.
- unified the image indexing, making sector access format agnostic.
- added flag to ignore second side (very useful for double sided disks/images that 
  have been reformatted single sided).
- added a struct containing the disk geometry, based on the imageIndex. 

New in 0.6:
- cleaned up code
- replaced raw hex dumps by neat hex & ascii
- structured the check code and extracted the boot sector check

New in 0.5:
- slightly improved help.
- replaced fixed DAM location by a dynamic search

New in 0.4:
- completed DMK parsing.

New in 0.3:
- improved DMK parsing, but single byte sectors only.	   
	   
fjkraan@xs4all.nl, 2012-12-09