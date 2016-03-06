CFLAGS=-DDEBUG=3

epspd : epspd.o cmd.o
	cc -o epspd epspd.o cmd.o -W

cmd.o : cmd.c fdc.h epsp.h
	cc -c cmd.c -W

epspd.o : epspd.c fdc.h epsp.h
	cc -c epspd.c -W

clean:
	rm -f *.o ; rm -f epspd

strip:
	strip epspd ; strip

package:
	tar cvzf ../vfloppy14.tgz ./Makefile ./README ./gpl.txt ./*.c ./*.h ./images/*.vfd ./vfrun.sh ./vfman/Makefile ./vfman/*.c ./vfman/*.h ./doc/*.*
