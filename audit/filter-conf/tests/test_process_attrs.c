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
**  FILE       : test_process_attrs.c
**
**  PURPOSE    : Used to test the audit filter configuration.
**
**  DESCRIPTION: This file contains various routines used to test
**  the filter configuration support of process attributes.  The
**  functions build into the laus_test framework to provide a basis
**  to verify that the Linux Audit System accurately logs the event
**  based upon the filter configuration.
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

#include <sys/wait.h>
#include <signal.h>


/**********************************************************************
**  Function: test_process_login_uid
**    Tests the login-uid process attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Forks a child process which executesa setreuid system call
**      with pre-determined uid's and then exits.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_process_login_uid(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	uid_t r_uid = uid;
	uid_t e_uid = uid;

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
		// Change the uid's
		rc = setreuid(r_uid, e_uid);

		// Generate an exit record, maybe
		exit(0);
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
	ld->msg_type = AUDIT_MSG_EXIT;
	ld->msg_ruid = r_uid;
	ld->msg_euid = e_uid;
	ld->msg_suid = e_uid;
	ld->msg_fsuid = e_uid;
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


/**********************************************************************
**  Function: test_process_uid
**    Tests the uid process attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Forks a child process which executesa setreuid system call
**      with pre-determined uid's and then exits.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_process_uid(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	uid_t r_uid = uid;
	uid_t e_uid = 0;

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
		// Change the uid's
		rc = setreuid(r_uid, e_uid);

		// Generate an exit record, maybe
		exit(0);
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
	ld->msg_type = AUDIT_MSG_EXIT;
	ld->msg_ruid = r_uid;
	ld->msg_euid = e_uid;
	ld->msg_suid = e_uid;
	ld->msg_fsuid = e_uid;
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


/**********************************************************************
**  Function: test_process_gid
**    Tests the gid process attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Executes the setreuid system call with pre-determined uid's.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_process_gid(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	gid_t r_gid = gid;
	gid_t e_gid = 0;

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
		// Change the gid's
		rc = setregid(r_gid, e_gid);

		// Generate an exit record, maybe
		exit(0);
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
	ld->msg_type = AUDIT_MSG_EXIT;
	ld->msg_rgid = r_gid;
	ld->msg_egid = e_gid;
	ld->msg_sgid = e_gid;
	ld->msg_fsgid = e_gid;
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


/**********************************************************************
**  Function: test_process_dumpable_on
**    Tests the dumpable process attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Forks a child process which exits immediately (dumpable flag
**      should remain on).
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_process_dumpable_on(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	pid_t pid = 0;
	int status = 0;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Fork the process to isolate the dumpable flag
	printf5("Preparing to fork child process\n");
	pid = fork();
	if (pid < 0) {
		printf1("ERROR: fork failed - errno=%d %s\n", errno, strerror(errno));
		rc = -1;
		goto EXIT;
	}
	else if (pid == 0) {
		// Child process
		// Generate an exit record, maybe
		exit(0);
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


/**********************************************************************
**  Function: test_process_dumpable_off
**    Tests the dumpable process attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Forks a child process which performs a setreuid with
**      pre-determined uid's and then exits (dumpable flag should
**      be off).
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_process_dumpable_off(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	uid_t r_uid = 0;
	uid_t e_uid = uid;

	pid_t pid = 0;
	int status = 0;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Fork the process to isolate the dumpable flag
	printf5("Preparing to fork child process\n");
	pid = fork();
	if (pid < 0) {
		printf1("ERROR: fork failed - errno=%d %s\n", errno, strerror(errno));
		rc = -1;
		goto EXIT;
	}
	else if (pid == 0) {
		// Child process
		// Generate a syscall record, maybe, via setreuid
		//   (which should turn off dumpable flag)
		rc = setreuid(r_uid, e_uid);
		if (rc)
			printf1("ERROR: setreuid failed - errno=%d %s\n", errno, strerror(errno));

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
	if (WEXITSTATUS(status) != 0) {
		printf1("ERROR: child exited with an error\n");
		rc = -1;
		goto EXIT;
	}

	// Set the data
	ld->msg_type = AUDIT_MSG_EXIT;
	ld->msg_ruid = r_uid;
	ld->msg_euid = e_uid;
	ld->msg_suid = e_uid;
	ld->msg_fsuid = e_uid;
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


/**********************************************************************
**  Function: test_process_exit_normal
**    Tests the exitcode process attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Forks a child process which exits immediately.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_process_exit_normal(laus_data *ld, uid_t uid, gid_t gid) {

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
		// Generate an exit record, maybe, via exit
		exit(0);
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


/**********************************************************************
**  Function: test_process_exit_signal
**    Tests the exitcode process attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Forks a child process which is killed with a pre-determined
**      signal.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_process_exit_signal(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	pid_t pid = 0;
	int status = 0;


	// Do as much setup work as possible right here
	ld->msg_type = AUDIT_MSG_EXIT;

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
		printf1("ERROR: child did not receive kill signal\n");
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
