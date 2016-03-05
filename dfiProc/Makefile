# $Id: Makefile,v 1.11 2012/02/10 22:55:20 fjkraan Exp $

#CFLAGS= -O3 -Wall
CFLAGS= -Wall -W -Wshadow -Wstrict-prototypes -fno-strict-aliasing -Werror\
    -Wno-unused -Wno-parentheses -Wno-switch
CC= gcc
DPEXE= dfiProc
EXE= $(DPEXE)


all: $(EXE) 

progs: $(EXE)

dfiProc$E: dfiProcess.c decoder.c logger.c crc.c kind.c
	$(CC) $(CFLAGS) dfiProcess.c decoder.c logger.c crc.c kind.c -o $(DPEXE)

clean:
	$(RM) $(EXE) *.$O *~

veryclean: clean
	$(RM) *.exe *.obj *.o $(TXT)

setuid:
	chown root $(CWEXE)
	chmod 4755 $(CWEXE)
