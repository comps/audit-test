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
**  FILE   : post_syscall.c
**
**  PURPOSE: This file defines a function that will call a series of 
**           functions that execute to after the system call.
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

int postSysCall(struct audit_data *context, int resultErrno, int errorRC,
		int expectedErrno)
{
    int rc = 0;

    fprintf(stderr, "Setting end timestamp\n");
    context->end_time = time(NULL) + 2;	// MH: 2 seconds past current
    // time

    // su back to root
    fprintf(stderr, "setresuid to root\n");
    if ((rc = setresuid(0, 0, 0)) != 0) {
	fprintf(stderr, "Unable to setresuid to root: errno=%i\n", errno);
	goto EXIT;
    }

    fprintf(stderr, "setresgid to root\n");
    if ((rc = setresgid(0, 0, 0)) != 0) {
	fprintf(stderr, "Unable to setresgid to root: errno=%i\n", errno);
	goto EXIT;
    }
    // Save resulting errno into data structure
    if (context->u.syscall.exit == -1) {
	context->u.syscall.exit = resultErrno;
    }

    // Check if the syscall executed as expected.
    fprintf(stderr, "Checking to see if the system call executed as expected\n");
    if (context->u.syscall.exit == errorRC) {
	if (context->success) {
	    fprintf(stderr, "SYSCALL ERROR: unsuccessful in successful case:");
	    rc = 1;
	} else {
	    if (resultErrno == expectedErrno) {
		fprintf(stderr, "SYSCALL SUCCESS: unsuccessful in unsuccessful case:");
	    } else {
		fprintf
		    (stderr,
		     "SYSCALL ERROR: unsuccessful, but errno != expected (%i):",
		     expectedErrno);
		rc = 1;
	    }
	}
    } else {
	if (context->success) {
	    fprintf(stderr, "SYSCALL SUCCESS: successful in successful case:");
	} else {
	    fprintf(stderr, "SYSCALL ERROR: successful in unsuccessful case:");
	    rc = 1;
	}
    }
    printf(" exit=%i\n", context->u.syscall.exit);

EXIT:
    return rc;

}
