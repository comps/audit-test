/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of version 2 the GNU General Public License as
 *  published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>

/*
* s390 is not able to handle more than 5 syscall parameters
* need to pass the parameters in a struct
*/
#if defined(MACHINE_s390x) || defined(MACHINE_s390)
struct mmap_arg_struct {
    unsigned long addr;
    unsigned long len;
    unsigned long prot;
    unsigned long flags;
    unsigned long fd;
    unsigned long offset;
};
#endif

int main(int argc, char **argv)
{
    void *addr;
    int fd, rc;
    int result = TEST_SUCCESS;
    char filename[80];
#if defined(MACHINE_s390x) || defined(MACHINE_s390)
    struct mmap_arg_struct mmap_arg_struct;
    int prot = PROT_WRITE;
    int flags = MAP_SHARED;
#endif

    if (argc != 2) {
	fprintf(stderr, "Usage:\n%s <flags>\n", argv[0]);
	return TEST_ERROR;
    }

    sprintf(filename, "/tmp/mmap_pgoff.%d", getpid());

    if (!strcmp(argv[1], "FAIL")) {
        /* Use creat, get WRONLY fd, causes mmap2 to EACCES */
        fd = creat(filename, 0777);
        if (fd == -1) {
            perror("do_mmap_pgoff: create");
            return TEST_ERROR;
        }
    } else {	/* PASS */
        fd = open(filename, O_RDWR|O_CREAT|O_EXCL, 0777);
        if (fd == -1) {
            perror("do_mmap_pgoff: open");
            return TEST_ERROR;
        }
    }
    unlink(filename);	/* cleanup temp file */

    rc = write(fd, "Write some data to the file.", 24);
    if (rc == -1) {
        perror("do_mmap_pgoff: write");
        return TEST_ERROR;
    }

    errno = 0;

#if defined(MACHINE_s390x) || defined(MACHINE_s390)
    mmap_arg_struct.addr = 0;
    mmap_arg_struct.len = 10;
    mmap_arg_struct.prot = prot;
    mmap_arg_struct.flags = flags;
    mmap_arg_struct.fd = fd;
    mmap_arg_struct.offset = 0;
    addr = (void*)syscall(__NR_mmap2, &mmap_arg_struct);
#else
    addr = (void*)syscall(__NR_mmap2, NULL, 10, PROT_READ, MAP_PRIVATE, fd, 0);
#endif

    if (addr == MAP_FAILED) {
    	result = TEST_FAIL;
        fprintf(stderr, "%d %d %d\n", result, errno, getpid());
    } else {
#if defined(MACHINE_ppc64) || defined(MACHINE_i686)
        fprintf(stderr, "%d %d %d\n", result, (int) addr, getpid());
#else
        fprintf(stderr, "%d %lu %d\n", result, (unsigned long) addr, getpid());
#endif
    }

    return result;
} 
