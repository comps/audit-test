/* =======================================================================
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2005
 * Written by Aron Griffis <aron@hp.com>
 * 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of version 2 the GNU General Public License as
 *   published by the Free Software Foundation.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * ======================================================================= 
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int __fprintf_chk(FILE *stream, int flag, const char *format, ...)
{
    va_list ap;
    int status;
    char *s;
    static int fail_after = 0;

    if (strcmp(format, "%s\n") == 0) {
        va_start(ap, format);
        s = va_arg(ap, char*);
        va_end(ap);

        if (strncmp(s, "type=USER", 9) == 0 &&
            fail_after++ == 20)
        {
            errno = EIO;
            return -1;
        }
    }

    va_start(ap, format);
    status = vfprintf(stream, format, ap);
    va_end(ap);
    return status;
}
