/* Copyright (c) 2014 Red Hat, Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 2 the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include <sys/mman.h>

int main(int argc, char **argv)
{
    int exitval, result;
    size_t page_size = sysconf(_SC_PAGESIZE);
    void *buf;
    size_t size;
    int ret;

    if (argc != 2) {
        fprintf(stderr, "Usage:\n%s <mem_size>\n", argv[0]);
        return TEST_ERROR;
    }

    size = strtoul(argv[1], NULL, 10);

    /* allocates page aligned memory */
    if ((ret = posix_memalign(&buf, page_size, size)) != 0) {
        fprintf(stderr, "do_mlock: posix_memalign returned %d\n", ret);
        return TEST_ERROR;
    }

    errno = 0;
    exitval = mlock(buf, size);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    munlock(buf, size);
    free(buf);
    return result;
}
