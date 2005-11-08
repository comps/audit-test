/*********************************************************************
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
 **  FILE   : test_ioctl.c
 **
 **  PURPOSE: The test_ioctl() function builds into the laus_test
 **           framework to verify that the Linux Audit System accurately
 **           logs both successful and erroneous execution of the
 **           "ioctl" system call.
 **
 **           In the successful case, this function:
 **             1) open /dev/tty device 
 **             2) invoke ioctl syscall with TCGETA command 
 **             3) verify successful ioctl audit record is generated
 **
 **
 **            In the erroneous case, this function:
 **             1) open temporary file 
 **             2) invoke ioctl syscall using temporary file file descriptor
 **             3) verify unsuccessful audit for ENOTTY device is generated 
 **
 **  HISTORY:
 **    11/04 originated by Dan Jones (danjones@us.ibm.com)
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <termio.h>
#include <sys/ioctl.h>

int test_ioctl(laus_data *dataPtr)
{


    int rc = 0;
    int exp_errno = ENOTTY;
    int fd = 0;
    char *dev = "/dev/tty";
    char *notty = "/tmp/notty";
    char *path;
    char dummy[] = { 0 };
    int tcgeta = TCGETA;
    struct termio tio;

    // Set the syscall specific data
    dataPtr->laus_var_data.syscallData.code = AUDIT_ioctl;

    if (dataPtr->successCase) {
	// Create a file readable by test user if testing success case
	if ((fd = open(dev, O_RDWR, 0777)) == -1) {
	    printf1("ERROR: Cannot open tty device %s\n", dev);
	    goto EXIT;
	}
	path = dev;
    } else {
	// Create a file readable by test user if testing success case
	if ((fd = open(notty, O_CREAT, 0777)) == -1) {
	    printf1("ERROR: Cannot create test file %s\n", notty);
	    goto EXIT;
	}
	path = notty;
    }

    // Set up audit argument buffer
    if ((rc = auditArg3(dataPtr,
			AUDIT_ARG_PATH, strlen(path), path,
			AUDIT_ARG_IMMEDIATE, sizeof(int), &tcgeta,
			AUDIT_ARG_POINTER, 0, dummy)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }
    // Execute system call
    rc = dataPtr->laus_var_data.syscallData.result =
	syscall(__NR_ioctl, fd, TCGETA, &tio);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT;
    }



EXIT:

    close(fd);

    // cleanup
    if (!dataPtr->successCase) {
	// close dev for success case
	unlink(notty);
    }

    return rc;
}
