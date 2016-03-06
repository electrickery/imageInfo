SRCS=vf.c 
OBJS=$(SRCS:.c=.o)
CFLAGS=-DDEBUG=0 -Wall

all: vfread vfwrite

vfread: vfread.o $(OBJS)
	$(CC) -o vfread vfread.o $(OBJS) $(LDFLAGS)

vfwrite: vfwrite.o $(OBJS) 
	$(CC) -o vfwrite vfwrite.o $(OBJS) $(LDFLAGS)

depend: $(SRCS)
	for i in $(SRCS);do $(CC) -MM $(CPPFLAGS) $$i;done > .depend

clean:
	rm -f *.o vfread vfwrite

strip:
	strip vfread ; strip vfwrite
