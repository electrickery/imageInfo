SRCS=cmd.c epspd.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-DDEBUG=1

all: $(OBJS) 
	cc $(CFLAGS) -o epspd $(OBJS)

depend: $(SRCS)
	for i in $(SRCS);do $(CC) -MM $(CPPFLAGS) $$i;done > .depend

clean:
	rm -f *.o
