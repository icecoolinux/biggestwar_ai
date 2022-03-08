    
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    mkfifo("/tmp/test", 0666);
	
	/*
	int fd = open("/tmp/test", O_WRONLY);
	
	char buf[200];
	float* numeros = new float[10000000];
	for(int i=0; i<10000000; i++)
		numeros[i] = ((float)i)*1.5f;

	//read(fd_tocpp, buf, 200);
	int sended = write(fd, numeros, sizeof(float)*10000000);
	printf("%d\n", sended);
	close(fd);
	
	
	write(fd, "0\n", 3);
	write(fd, "1\n", 3);
	close(fd);
	*/
	
	
	int fd = open("/tmp/test", O_RDONLY);
	
	char buf[200];

	int bytes = read(fd, buf, 200);
	close(fd);
	printf("%s\n", buf);
	printf("%d\n", bytes);
	
}
