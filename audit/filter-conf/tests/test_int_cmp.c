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
**  FILE       : test_int_cmp.c
**
**  PURPOSE    : Used to test the audit filter configuration.
**
**  DESCRIPTION: This file contains various routines used to test
**  the filter configuration integer comparison support.  The
**  functions build into the laus_test framework to provide a basis
**  to verify that the Linux Audit System accurately logs the event
**  based upon the filter configuration.
**
**  Each function will documents its execution.
**
**  HISTORY    :
**    08/03 Originated by Tom Lendacky (toml@us.ibm.com)
**    05/04 Adapted for EAL4 by Dan Jones (danjones@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include "filterconf.h"
#include "filter_call_utils.h"

#include <sys/wait.h>


/**********************************************************************
**  Function: test_int_cmp
**    Tests the integer comparison support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Set the gid of the process to a predetermined value (10).
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_int_cmp(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	//int status = 0;
	int testgid = TEST_GID;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	printf5("Calling auditArg1\n");
	if ((rc = auditArg1(ld,
			AUDIT_ARG_IMMEDIATE, sizeof(testgid), &testgid)) != 0) {
		printf1("ERROR: auditArg1 failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Set the process gid to the test value
	printf5("Setting process gid to test value\n");
	if ((rc = setgid(testgid)) != 0) {
	  printf1("ERROR: setgid failed - rc=%d\n", rc);
	  goto EXIT;
	}

	// Now put it back to root
	printf5("Resetting process gid to root\n");
	if ((rc = setgid(0)) != 0) {
	  printf1("ERROR: setgid failed - rc=%d\n", rc);
	  goto EXIT;
	}

	// Set the data
	ld->msg_type = AUDIT_MSG_SYSCALL;
	ld->msg_egid = testgid;
	ld->msg_rgid = testgid;
	ld->msg_sgid = testgid;
	ld->msg_fsgid = testgid;
	ld->laus_var_data.syscallData.code = AUDIT_setgid;
	ld->laus_var_data.syscallData.result = 0;
	ld->laus_var_data.syscallData.resultErrno = 0;

	// Do post-system call work
	printf5("Calling post_filter_call\n");
	if ((rc = post_filter_call(ld)) != 0) {
		printf1("ERROR: post_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

EXIT:
	// Do cleanup work here
	printf5("Returning from test - rc=%d\n", rc);

	return rc;
}


/**********************************************************************
**  Function: test_mask_cmp
**    Tests the mask comparison support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Forks a child process which is killed with a pre-determined
**      signal.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_mask_cmp(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	pid_t pid = 0;
	int status = 0;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Fork the process
	printf5("Preparing to fork child process\n");
	pid = fork();
	if (pid < 0) {
		printf1("ERROR: fork failed - errno=%d %s\n", errno, strerror(errno));
		rc = -1;
		goto EXIT;
	}
	else if (pid == 0) {
		// Child process
		sleep(60);

		// Should only get here if kill failed
		exit(0);
	}
	// Use new PID
	printf5("Fork successful - pid=%d\n", pid);
	ld->msg_pid = pid;

	// Generate an exit record, maybe, via kill
	printf5("Killing child process with SIGABRT\n");
	if ((rc = kill(pid, SIGABRT)) != 0) {
		printf1("ERROR: kill failed - errno=%d %s\n", errno, strerror(errno));
		goto EXIT;
	}

	printf5("Waiting for child process\n");
	wait(&status);
	if (WIFEXITED(status)) {
		printf1("ERROR: child did not exit abnormally (via kill)\n");
		rc = -1;
		goto EXIT;
	}

	// Set the data
	ld->msg_type = AUDIT_MSG_EXIT;
	ld->laus_var_data.exitData.code = status;

	// Do post-system call work
	printf5("Calling post_filter_call\n");
	if ((rc = post_filter_call(ld)) != 0) {
		printf1("ERROR: post_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

EXIT:
	// Do cleanup work here
	printf5("Returning from test - rc=%d\n", rc);

	return rc;
}
