/* objreuse-ftruncate.c - must zero-fill (or fail) when enlarging files
 *
 * $Id: objreuse-ftruncate.c,v 1.1 2004/06/23 22:48:21 danjones Exp $
 *
 * Purpose: verify that the object reuse mechanism works as documented
 *      in the case of accessing uninitialized areas of disk files.
 *	The ftruncate() system call is normally used to shorten files,
 *	but it is also permissible to request that a file be enlarged.
 *	The kernel has the choice of either refusing the request (leaving
 *	the file size unchanged), or extending the file size, in which
 *	case the added area must be zero-filled.
 *      from previous use.
 *
 * Method: Create a temporary file, and attempt to increase the size
 *	using the ftruncate() call. Verify that either end-of-file or
 *	zero bytes are returned when trying to read back data.
 *      Check different allocation sizes to catch off-by-one or
 *	pagesize-related cases.
 *
 * Expected result: exit code 0 indidates that all tests worked as
 *      expected.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "testmacros.h"

void
verify_zeroes(int fd, int size)
{
	int j;

	SYSCALL( lseek(fd, 0, SEEK_SET) );

	for (j=0; j<size; j++) {
		unsigned char c = 1;
		int r;

		SYSCALL( r=read(fd, &c, 1) );

		/* If ftruncate can't extend files, we get EOF,
		 * otherwise the returned character must be NUL */
		DIE_IF( r > 0 && c != 0);
	}
}

int
main(int argc, char *argv[])
{
	int fd;
	int increments[]={
		1, 2, 3, 4, 15, 16, 17, 1023, 1024, 1025,
		4095, 4096, 4097, 16383, 16384, 16385,
		(1<<17) - 1, (1<<17), (1<<17) + 1,
	};
	int i;
	FILE *tmpf;

	DIE_UNLESS( tmpf = tmpfile() );
	SYSCALL ( fd = fileno(tmpf) );

	for (i=0; i < sizeof(increments)/sizeof(increments[0]); i++) {
		char zero = 0;
		SYSCALL( ftruncate(fd, increments[i]) );
		SYSCALL( write(fd, &zero, 1) );
		verify_zeroes(fd, increments[i]+1);
	}

	fclose(tmpf);
        printf("%s: PASS\n", __FILE__);
	return 0;
}
