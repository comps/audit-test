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
**  FILE   : audit_start.c
**
**  PURPOSE: This file defines a function that will start the auditing
**           system that is going to be tested.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**    11/03 Modified to use /etc/init.d/audit script by Michael A. Halcrow
**          <mike@halcrow.us>
**
**********************************************************************/

#include "includes.h"

/*
** Start auditing system calls
**
*/
int laus_start()
{

    int rc = 0;
    printf4("audit_start()\n");

    if ((rc = system("/etc/init.d/audit start")) == -1) {
	printf1("ERROR: Unable to start auditd; errno=%i\n", errno);
	goto EXIT_ERROR;
    }

    sleep(2);

EXIT_ERROR:
    return rc;
}
