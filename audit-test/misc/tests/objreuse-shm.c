/* objreuse-shm.c - shared memory allocated w/ shmget() must be zeroed by
 * kernel
 *
 * $Id: objreuse-shm.c,v 1.1 2004/06/23 22:48:21 danjones Exp $
 *
 * Purpose: verify that the object reuse mechanism works as documented
 * 	in the case of shared memory returned by the kernel. All shared
 *	memory made available must contain zeroes and not any data that
 *	may exist from previous use.
 *
 * Method: allocate shared memory via the shmget() system call and verify
 *      that the contents are all zero bytes. Check different allocation
 *	 sizes.
 *
 * Expected result: exit code 0 indidates that all tests worked as
 *	expected.
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <linux/sysctl.h>
#include <linux/unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "testmacros.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#ifndef PAGE_SIZE
  #define PAGE_SIZE getpagesize()
#endif

static char* shmmaxfile = "/proc/sys/kernel/shmmax";

void verify_zeroes(char *buf, int size)
{
	int j;

	for (j=0; j<size; j++) {
		DIE_IF(buf[j] != 0);
	}

	return;
}

int main(int argc, char *argv[])
{

  int i, fd, rc = 0;
  int shmid;
  void *shmaddr;
  char old_shmmax[20], new_shmmax[20];
  struct shmid_ds shmdata;

  /* check for failures */
  int failed = 0;

  /* shared memory segment sizes to test */
  size_t increments[] = {
	(1 * PAGE_SIZE) - 1,
        (1 * PAGE_SIZE),
        (1 * PAGE_SIZE) + 1,
	(2 * PAGE_SIZE) - 1,
        (2 * PAGE_SIZE),
        (2 * PAGE_SIZE) + 1,
	(1024 * PAGE_SIZE) - 1,
        (1024 * PAGE_SIZE),
        (1024 * PAGE_SIZE) + 1,
	(2048 * PAGE_SIZE) - 1,
        (2048 * PAGE_SIZE),
        (2048 * PAGE_SIZE) + 1,
	(4096 * PAGE_SIZE) - 1,
        (4096 * PAGE_SIZE),
        (4096 * PAGE_SIZE) + 1,
	(8192 * PAGE_SIZE) - 1,
        (8192 * PAGE_SIZE)
  };

  memset(old_shmmax, '\0', sizeof(old_shmmax));
  memset(new_shmmax, '\0', sizeof(new_shmmax));

  /* save the value of current SHMMAX */
  fd = open(shmmaxfile, O_RDWR);
  DIE_IF (fd < 0);

  rc = read(fd, old_shmmax, sizeof(old_shmmax));
  if (rc == -1) {
    printf("%s: WARN unable to determine kernel.shmmax\n", __FILE__);
    failed = 1;
  }

  /* set the value of SHMMAX */
  sprintf (new_shmmax, "%zd\n", increments[ARRAY_SIZE(increments) - 1]);
  rc = write(fd, new_shmmax, sizeof(new_shmmax));
  if (rc == -1) {
    printf("%s: WARN unable to set kernel.shmmax, errno = %d\n", __FILE__, errno);
    failed = 1;
  }
  close(fd);

  /* run the test */
  for (i=0; i < ARRAY_SIZE(increments); i++) {
    SYSCALL( shmid = shmget(IPC_PRIVATE, increments[i], S_IRUSR) );
    SYSCALL( shmaddr = shmat(shmid, NULL, 0) );
    SYSCALL( shmctl(shmid, IPC_STAT, &shmdata) );
    verify_zeroes((char *)shmaddr, shmdata.shm_segsz);
    SYSCALL(shmdt(shmaddr));
  }

  /* restore the value of SHMMAX */
  fd = open (shmmaxfile, O_RDWR);
  rc = write(fd, old_shmmax, sizeof(old_shmmax));
  if (rc == -1) {
    printf("%s: WARN unable to reset kernel.shmmax\n", __FILE__);
    failed = 1;
  }
  close(fd);

  if (!failed)
    printf("%s: PASS\n", __FILE__);
  return 0;
}
