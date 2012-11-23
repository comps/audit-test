/* objreuse-brk.c - memory allocated w/ brk() must be zeroed by kernel
 *
 * $Id: objreuse-brk.c,v 1.1 2004/06/23 22:48:21 danjones Exp $
 *
 * Purpose: verify that the object reuse mechanism works as documented
 * 	in the case of memory space returned by the kernel. All memory
 *	made available must contain zeroes and not any data that may
 *	from previous use.
 *
 * Method: allocate memory via the brk() system call and verify that the
 *	contents are all zero bytes. Check different allocation sizes
 *	to catch off-by-one or pagesize-related cases.
 *
 * Expected result: exit code 0 indidates that all tests worked as
 *	expected.
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "testmacros.h"

void
verify_zeroes(char *buf, int size)
{
	int j;

	for (j=0; j<size; j++) {
		DIE_IF(buf[j] != 0);
	}
}

int
main(int argc, char *argv[])
{
	char *obrk;

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
	int i;

	obrk = sbrk(0);

	for (i=0; i < sizeof(increments)/sizeof(increments[0]); i++) {
		SYSCALL(brk(obrk + increments[i]));
		verify_zeroes(obrk, increments[i]);
		SYSCALL(brk(obrk));
	}

	printf("%s: PASS\n", __FILE__);
	return 0;
}
