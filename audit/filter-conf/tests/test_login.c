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
**  FILE       : test_login.c
**
**  PURPOSE    : Used to test the audit filter configuration.
**
**  DESCRIPTION: This file contains various routines used to test
**  the filter configuration support of login messages.  The
**  functions build into the laus_test framework to provide a basis
**  to verify that the Linux Audit System accurately logs the system
**  call based upon the filter configuration.
**
**  Each function will documents its execution.
**
**  HISTORY    :
**    08/03 Originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include "filterconf.h"
#include "filter_call_utils.h"

#include <laus.h>
#include <sys/wait.h>


extern char test_exe[PATH_MAX];


/**********************************************************************
**  Function: test_login
**    Tests the login support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Executes the system call with a pre-determined aurun command
**      (to simulate a login).
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_login(laus_data *ld, uid_t uid, gid_t gid) {

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
		// Generate a login record, maybe
		if ((rc = laus_open(NULL) != 0)) {
			printf1("ERROR: laus_open failed - errno=%d %s\n", errno, laus_strerror(errno));
		}
		if ((rc = laus_detach() != 0)) {
			printf1("ERROR: laus_detach failed - errno=%d %s\n", errno, laus_strerror(errno));
		}
		if ((rc = laus_attach() != 0)) {
			printf1("ERROR: laus_attach failed - errno=%d %s\n", errno, laus_strerror(errno));
		}
		if ((rc = laus_setauditid() != 0)) {
			printf1("ERROR: laus_setauditid failed - errno=%d %s\n", errno, laus_strerror(errno));
		}
		if ((rc = laus_setsession(ld->msg_ruid, NULL, NULL, NULL)) != 0) {
			printf1("ERROR: laus_setsession failed - errno=%d %s\n", errno, laus_strerror(errno));
		}

		exit(rc);
	}
	// Use new PID
	printf5("Fork successful - pid=%d\n", pid);
	ld->msg_pid = pid;

	printf5("Waiting for child process\n");
	wait(&status);
	if (!WIFEXITED(status)) {
		printf1("ERROR: child exited abnormally\n");
		rc = -1;
		goto EXIT;
	}

	// Set the data
	ld->msg_type = AUDIT_MSG_LOGIN;
	ld->msg_login_uid = ld->msg_ruid;
	ld->laus_var_data.loginData.uid = ld->msg_ruid;
	strcpy(ld->laus_var_data.loginData.hostname, "");
	strcpy(ld->laus_var_data.loginData.address, "");
	strcpy(ld->laus_var_data.loginData.terminal, "");
	strcpy(ld->laus_var_data.loginData.executable, test_exe);

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
