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
#include <keyutils.h>

int main(int argc, char **argv)
{
    long exitval, result;
    long keyring;

    if (argc != 3) {
        fprintf(stderr, "Usage:\n%s <description> <keyring>\n", argv[0]);
        return TEST_ERROR;
    }

    if (!strcmp(argv[2], "KEY_SPEC_SESSION_KEYRING"))
        keyring = KEY_SPEC_SESSION_KEYRING;
    else if (!strcmp(argv[2], "KEY_SPEC_USER_KEYRING"))
        keyring = KEY_SPEC_USER_KEYRING;
    else if (!strcmp(argv[2], "KEY_SPEC_USER_SESSION_KEYRING"))
        keyring = KEY_SPEC_USER_SESSION_KEYRING;
    else {
        fprintf(stderr, "Invalid <keyring> argument\n");
        return TEST_ERROR;
    }

    errno = 0;
    exitval = syscall(__NR_request_key, "user", argv[1], NULL, keyring);
    result = exitval < 0;

    fprintf(stderr, "%ld %ld %d\n", result, result ? errno : exitval, getpid());
    return result;
}
