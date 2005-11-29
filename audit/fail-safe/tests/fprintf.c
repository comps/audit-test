/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
 * Written by Aron Griffis <aron@hp.com>
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   with this package; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * ======================================================================= 
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int fprintf(FILE *stream, const char *format, ...)
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
