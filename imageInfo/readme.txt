Yet another image interpreter

This program identifies disk images for TRS-80 Model I, III and 4 machines. Only
JV1, JV3 and DMK are currently supported.

Maybe I'll directory reading later.

usage: -f <image file>
       -h              - help and version
       -v <log level>  - verbosity (0-4), default 1
       -?              - help and version

New in 0.4:
- completed DMK parsing.

New in 0.3:
- improved DMK parsing, but single byte sectors only.	   
	   
fjkraan@xs4all.nl, 2012-12-06