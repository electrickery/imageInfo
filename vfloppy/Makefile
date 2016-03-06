SRCS=cmd.c 
OBJS=$(SRCS:.c=.o)
CFLAGS=-DDEBUG=3 -Wall

epspd: $(OBJS) 
	$(CC) $(LDFLAGS) -o epspd $(OBJS)

vformat: .c=.o
	$(CC) $(LDFLAGS) -o vformat .c=.o

depend: $(SRCS)
	for i in $(SRCS);do $(CC) -MM $(CPPFLAGS) $$i;done > .depend

clean:
	rm -f *.o ; rm -f epspd vformat

strip:
	strip epspd ; strip vformat

package:
	tar cvzf ../vfloppy13.tgz ./Makefile ./README ./gpl.txt ./*.c ./*.h ./*.vfd ./vfrun.sh ./vfman/Makefile ./vfman/*.c ./vfman/*.h ./doc/*
