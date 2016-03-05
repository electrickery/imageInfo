Yet another image interpreter

This program identifies disk images for TRS-80 Model I, III and 4 machines. Only
JV1, JV3 and DMK are currently supported.

Maybe I'll directory reading later.

usage: -f <image file>
       -h              - help and version
       -v <log level>  - verbosity (0-4), default 1
                         0 - ERROR; 1 - WARN; 2 - INFO; 3 - DEBUG; 4 - TRACE; 5 - BYTES 5
       -?              - help and version

New in 0.5:
- slightly improved help.
- replaced fixed DAM location by a dynamic search

New in 0.4:
- completed DMK parsing.

New in 0.3:
- improved DMK parsing, but single byte sectors only.	   
	   
fjkraan@xs4all.nl, 2012-12-09