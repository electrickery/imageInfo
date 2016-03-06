#include <stdio.h>
#include <fcntl.h>

main(int argc,char **argv)
{
	int f;
	int i;
	unsigned char buff[4097];

	if (argc<2) 
	{
		fprintf(stderr,"Usage: %s <filename>\n",argv[0]);
		exit(0);
	}
	
	memset(buff,0xe5,4096);
	f=open(argv[1],O_WRONLY|O_CREAT,0666);
	if (f<0) exit(0);
	for (i=0;i<128;i++)
		write(f,buff,4096);
	close(f);
}
