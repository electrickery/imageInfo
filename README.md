# Yet another image interpreter

This program is incomplete.

The program identifies disk images for TRS-80 Model I, III and 4 machines. Only
formats JV1, JV3 and DMK are currently supported. On OS level it knows TRSDOS, LDOS and NEWDOS/80/86/90
formatted disks. If a know image format is identified but no known format is identified, an 
HEX/ASCII dump of the boot sector is printed.

For the recognized formats a directory listing is possible. 

usage: 

       -d display directory tracks
       -f <image file>
       -h              - help and version
       -s force single sided interpretation
       -v <log level>  - verbosity (0-4), default 1
                         0 - ERROR; 1 - WARN; 2 - INFO; 3 - DEBUG; 4 - TRACE; 5 - BYTES 5
       -?              - help and version

The functionality is limited, but the code contains a lot of (machine readable) structural information. It is old
code, but might be usable for someone.

The original site is http://electrickery.nl/comp/divcomp/imageInfo/, but somehow I forgot to make a link to it.

The licence is 2-clause BSD.

fjkraan@electrickery.nl, 2022-12-30
