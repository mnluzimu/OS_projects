#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>
#include <stdio.h>

int main()
{
   	long t;
	char buf[50];
	static const char s[] = "Hello, world!\n";
	long buf_len = 50;
	t = syscall(548, buf, buf_len); //test if buffer enough
	if(t == -1) {
		printf("return value %ld\n", t);
		printf("buffer not large enough!\n");
	}
	else {
		printf("return value %ld\n", t);
		printf("success!\n");
		printf("buffer: %s\n", s);
	}
	
	buf_len = 5;
	t = syscall(548, s, buf_len);
	if(t == -1) {
		printf("return value %ld\n", t);
		printf("buffer not large enough!\n");
	}
	else {
		printf("return value %ld\n", t);
		printf("success!\n");
		printf("buffer: %s\n", s);
	}
	while(1);
	return 0;
}

