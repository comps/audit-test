/* objreuse-mmap.c - mapped memory  must be zeroed by kernel
 *
 * $Id: objreuse-mmap.c,v 1.1 2004/06/23 22:48:21 danjones Exp $
 *
 * Purpose: verify that the object reuse mechanism works as documented
 * 	in the case of a mapped memory returned by the kernel. All memory
 *	made available must contain zeroes and not any data that may
 *	from previous use.
 *
 * Method: allocate mapped memory via the mmap() system call and verify the
 *	contents are all initialized to zeros.
 *
 * Expected result: exit code 0 indidates that all tests worked as
 *	expected.
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "testmacros.h"

#define FILENAME "mmapfile"

void verify_zeroes(char *buf, int size)
{
	int j;

	for (j=0; j<size; j++) {
		DIE_IF(buf[j] != 0);
	}
}

int main(int argc, char *argv[])
{
	int fd;
	int i;
	char *buff;
	void *mem;

	int increments[]={
		1, 2, 3, 4, 15, 16, 17, 1023, 1024, 1025,
		4095, 4096, 4097, 16383, 16384, 16385,
		(1<<11) - 1, (1<<11), (1<<11) + 1,
		(1<<12) - 1, (1<<12), (1<<12) + 1,
		(1<<20) - 1, (1<<20), (1<<20) + 1,
		(1<<21) - 1, (1<<21), (1<<21) + 1,
		(1<<24) - 1, (1<<24), (1<<24) + 1,
		(1<<25) - 1, (1<<25), (1<<25) + 1,
	};

	for (i=0; i < sizeof(increments)/sizeof(increments[0]); i++) {
	  SYSCALL( fd = open(FILENAME, O_CREAT | O_RDWR,
			     S_IRWXU | S_IRWXG | S_IRWXO) );
	  buff = (char *)malloc(increments[i]);
	  memset(buff, '\0', increments[i]);
	  SYSCALL( write(fd, buff, increments[i]) );
	  SYSCALL( mem = mmap(0, increments[i], PROT_READ | PROT_WRITE,
			      MAP_PRIVATE, fd, 0) );
	  verify_zeroes((char *)mem, increments[i]);
	  SYSCALL( munmap(mem, increments[i]) );
	  free(buff);
	  unlink(FILENAME);
	}

        printf("%s: PASS\n", __FILE__);
	return 0;
}
