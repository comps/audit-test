/**********************************************************************
**   Copyright (C) International Business Machines  Corp., 2003
**
**   This program is free software;  you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY;  without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
**   the GNU General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program;  if not, write to the Free Software
**   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**
**
**
**  FILE   : get_pts.c
**
**  PURPOSE: This file will write the tty device path to the 
**           file specified by argv[1].
**
**  HISTORY:
**    05/04 originated by Daniel H. Jones <danjones@us.ibm.com> 
**
**********************************************************************/

#include "includes.h"

int main(int argc, char **argv)
{

    int rc = 0;
    int fd;
    char *fd0 = "/proc/self/fd/0";
    char pts[40];

    memset(pts, '\0', sizeof(pts));
    if ((rc = readlink(fd0, pts, sizeof(pts))) == -1) {
	goto EXIT;
    }

    if ((fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY,
		   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))
	== -1) {
	rc = fd;
	goto EXIT;
    }

    rc = write(fd, pts, strlen(pts));
    close(fd);
EXIT:
    return rc;
}
