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
 **  FILE   : test_utimes.c
 **
 **  PURPOSE: The test_utimes() function builds into the laus_test
 **           framework to verify that the Linux Audit System accurately
 **           logs both successful and erroneous execution of the
 **           "utimes" system call.
 **
 **           In the successful case, this function:
 **             1) Creates the temporary file
 **             2) Creates the timeval data structure
 **             3) Executes the "utimes" system call
 **
 **           The successful case executes the expected conditions
 **           described by the "utimes" system call manpage.  That is,
 **           the utimes() function is called using a valid filename 
 **           and actually changes the access and modified timestamps
 **           on the file.
 **
 **            In the erroneous case, this function:
 **             1) Creates the temporary file
 **             2) Creates the timeval data structure
 **             3) Executes the "utimes" system call as helper_uid
 **
 **            The erroneous case executes the faulty conditions
 **            described by the "EPERM" errno.
 **            That is, the utimes() function is called on NULL.
 **
 **
 **  HISTORY:
 **    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
 **    10/03 furthered by Kylene J. Smith (kylene@us.ibm.com)
 **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
 **    05/05 modified utime() => utimes(): Amy Griffis <amy.griffis@hp.com>
 **
 **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <utime.h>
#include <sys/time.h>

/* Skip test on platforms that don't support the call */
#ifdef __NR_utimes

#if (defined(__S390X) || defined(__X86_64) || defined(__PPC64)) && defined(__MODE_32)
// This is needed for accurate processing of headers on 64bit platforms when test suite
//   is compiled in 31/32bit mode but running on a 64bit kernel (emulation).
//   auditd is running in 64bit mode but compilation of the test suite yields
//   data structures whose sizes are different.
struct timespec_on_disk {	// edited from /usr/include/time.h
    __u64 tv_sec;		/* Seconds.  */
    __u64 tv_nsec;		/* Nanoseconds.  */
};
#else
#define timespec_on_disk timespec
#endif

/*
 ** execute a utimes operation
 */
int test_utimes(laus_data *dataPtr)
{

    int rc = 0;
    int exp_errno = EPERM;
    char *fileName = NULL;
    struct timespec_on_disk mod_time, acc_time;
    struct timeval utbuf[2];


    // Set the syscall specific data
    dataPtr->laus_var_data.syscallData.code = AUDIT_utimes;

    // Create the file 
    if ((rc = createTempFile(&fileName, S_IRWXU | S_IRWXG | S_IRWXO,
			     dataPtr->msg_euid, dataPtr->msg_egid)) == -1) {
	printf1("ERROR: Cannot create file %s\n", fileName);
	goto EXIT;
    }
    // utimes setup
    acc_time.tv_sec = 30;
    acc_time.tv_nsec = 0;
    mod_time.tv_sec = 10;
    mod_time.tv_nsec = 0;
    memcpy(&(utbuf[0].tv_sec), &acc_time, sizeof(acc_time));
    memcpy(&(utbuf[1].tv_sec), &mod_time, sizeof(mod_time));

    if (!dataPtr->successCase) {
	dataPtr->msg_euid = dataPtr->msg_ruid = dataPtr->msg_fsuid = helper_uid;
    }

    // Set up audit argument buffer
    if ((rc = auditArg3(dataPtr,
			AUDIT_ARG_PATH, strlen(fileName), fileName,
			AUDIT_ARG_POINTER, sizeof(struct timespec_on_disk),
			&acc_time, AUDIT_ARG_POINTER,
			sizeof(struct timespec_on_disk), &mod_time)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(dataPtr)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    dataPtr->laus_var_data.syscallData.result = utimes(fileName, utbuf);

    // Do post-system call work
    if ((rc = postSysCall(dataPtr, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
    // utimes cleanup
    if ((rc = unlink(fileName)) != 0) {
	printf1("ERROR: Unable to remove file %s: errno=%i\n", fileName, errno);
	goto EXIT;
    }

EXIT:
    if (fileName)
	free(fileName);
    return rc;
}

#endif /* __NR_utimes */
