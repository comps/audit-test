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
**  FILE   : pre_syscall.c
**
**  PURPOSE: This file defines a function that will call a series of 
**           functions that execute to prepare the test environment for
**           the system call.
**
**
**  HISTORY:
**    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**                    and Michael Halcrow (mhalcrow@cs.byu.edu)
**    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
**
**********************************************************************/

#include "includes.h"
#include <time.h>

int preSysCall(struct audit_data *context)
{
    int rc = 0;

    // su to test user
    fprintf(stderr, "setresgid to %i, %i, %i\n", context->egid, context->egid, 0);
    if ((rc = setresgid(context->egid, context->egid, 0)) != 0) {
	fprintf(stderr, "Unable to setresgid to %i, %i, %i: errno=%i\n",
		context->egid, context->egid, 0, errno);
	goto EXIT;
    }
    fprintf(stderr, "setresuid to %i, %i, %i\n", context->euid, context->euid, 0);
    if ((rc = setresuid(context->euid, context->euid, 0)) != 0) {
	fprintf(stderr, "Unable to setresuid to %i, %i, %i: errno=%i\n",
		context->euid, context->euid, 0, errno);
	goto EXIT;
    }

    fprintf(stderr, "Setting begin timestamp\n");
    context->begin_time = time(NULL) - 2;	// MH: Start two seconds
    // before current time
    fprintf(stderr, "Performing system call\n");

EXIT:
    return rc;

}
