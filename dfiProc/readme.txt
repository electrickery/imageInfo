This is the readme.txt of the 0.2 version of the DFI-processor.
This tool reads image files produced by the DiscFerret and displays the 
interpreted data on screen. Eventually the output should be written to some
diskimage formats like DMK, JV1, JV3, D88, IMD, ...
Optionally data could be read directly from the DiscFerret.

For now, single and double density are supported. The core-interpreter
supports also MFM, RX02 and M2MFM, but only FM and MFM are tested. Auto detection 
is not working, so mixed density is not supported yet.

The core is not programmed by me but from the cw2dmk package by Tim Mann,
http://tim-mann.org/catweasel.html.

THe tool can be started by:
./NetBeansProjects/dfiProcess/dfiProc -d newdos80m1.dfi -s -v 4

the -s option skips every odd track. This is because the original disk is
35 tracks, and the drive used to read it 80 tracks. The supported DFI format
is version 1.1 (header DFE2).

2012-02-18,
Fred Jan Kraan
