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
#include <linux/kexec.h>

int main(int argc, char **argv)
{
    int exitval, result;
    unsigned long entry, flags;

    if (argc != 2) {
        fprintf(stderr, "Usage:\n%s <0xentry>\n", argv[0]);
        return TEST_ERROR;
    }

    flags = KEXEC_ARCH_DEFAULT;
    entry = strtoul(argv[1], NULL, 16);

    errno = 0;
    exitval = syscall(__NR_kexec_load, entry, 0, NULL, flags);
    result = exitval < 0;

    fprintf(stderr, "%d %d %d\n", result, result ? errno : exitval, getpid());
    return result;
}
