# $Id: Makefile,v 1.11 2012/02/10 22:55:20 fjkraan Exp $

#CFLAGS= -O3 -Wall
CFLAGS= -Wall -W -Wshadow -Wstrict-prototypes -fno-strict-aliasing -Werror\
    -Wno-unused -Wno-parentheses -Wno-switch
CC= gcc
DPEXE= imageInfo
EXE= $(DPEXE)


all: $(EXE) 

progs: $(EXE)

imageInfo$E: imageInfo.c crc.c
	$(CC) $(CFLAGS) imageInfo.c crc.c -o $(DPEXE)
	mv $(DPEXE) dist/Debug/GNU-Linux-x86/

clean:
	$(RM) $(EXE) *.$O *~

veryclean: clean
	$(RM) *.exe *.obj *.o $(TXT)

setuid:
	chown root $(CWEXE)
	chmod 4755 $(CWEXE)
