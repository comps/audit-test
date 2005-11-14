/**********************************************************************
 **   Copyright (C) 2003 International Business Machines Corp.
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
 **  FILE       : get_pid.c
 **
 **  PURPOSE    : To get the pid of a process from its /var/run entry
 **
 **  HISTORY    :
 **    08/2003 Originated by Michael A. Halcrow <mike@halcrow.us>
 **
 **********************************************************************/

#include "includes.h"

int getPid(char *executable)
{
    char pidFilename[255];
    FILE *fPtr;
    char buf[11];

    snprintf(pidFilename, 255, "/var/run/%s.pid", executable);

    if ((fPtr = fopen(pidFilename, "r")) == NULL) {
	fprintf(stderr, "Error: cannot open %s to get the process ID of the currently running instance of %s\n",
	     pidFilename, executable);
	return -1;
    }

    if (fgets(buf, 10, fPtr) == NULL) {
	fprintf(stderr, "Error: cannot read %s to get the process ID of the currently running instance of %s\n",
	     pidFilename, executable);
	return -1;
    }

    fclose(fPtr);

    return atoi(buf);
}
