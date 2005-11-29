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
**  FILE       : test_file_attrs.c
**
**  PURPOSE    : Used to test the audit filter configuration.
**
**  DESCRIPTION: This file contains various routines used to test
**  the filter configuration support of file attributes.  The
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


extern char test_devname[DEVNAME_LEN];


/**********************************************************************
**  Function: test_file
**    Tests the file-mode, file-dev, file-ino file attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Executes the open system call with a pre-determined file mode.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_file(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	int fd = 0;
#if defined(__powerpc64__) || defined(__s390x__) || defined(__x86_64__)
        int flags = O_RDONLY | O_LARGEFILE;
#else
        int flags = O_RDONLY;
#endif
	int mode = 0;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Set up audit argument buffer
	printf5("Calling auditArg3\n");
	if ((rc = auditArg3(ld,
			AUDIT_ARG_PATH, strlen(AUDIT_CONF), AUDIT_CONF,
			AUDIT_ARG_IMMEDIATE, sizeof(flags), &flags,
			AUDIT_ARG_IMMEDIATE, sizeof(mode), &mode)) != 0) {
		printf1("ERROR: auditArg3 failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Generate a syscall record, maybe, via open
	fd = open(AUDIT_CONF, flags, mode);

	// Set the data
	ld->msg_type = AUDIT_MSG_SYSCALL;
	ld->laus_var_data.syscallData.code = AUDIT_open;
	ld->laus_var_data.syscallData.result = fd;
	ld->laus_var_data.syscallData.resultErrno = errno;

	// Close the file if the open succeeded
	if (fd >= 0)
		close(fd);

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
**  Function: test_file_owner
**    Tests the file-uid and file-gid file attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Creates a temporary file with a pre-determined uid and gid.
**   3) Executes the open system call.
**   4) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_file_owner(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	char *file = NULL;
	int fd = 0;
#if defined(__powerpc64__) || defined(__s390x__) || defined(__x86_64__)
        int flags = O_RDONLY | O_LARGEFILE;
#else
        int flags = O_RDONLY;
#endif
	int mode = 0;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Create a temporary file
	file = init_tempfile(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, uid, gid);
	if (!file) {
	    rc = -1;
	    goto EXIT;
	}

	// Set up audit argument buffer
	printf5("Calling auditArg3\n");
	if ((rc = auditArg3(ld,
			AUDIT_ARG_PATH, strlen(file), file,
			AUDIT_ARG_IMMEDIATE, sizeof(flags), &flags,
			AUDIT_ARG_IMMEDIATE, sizeof(mode), &mode)) != 0) {
		printf1("ERROR: auditArg3 failed - rc=%d\n", rc);
		goto EXIT_CLEANUP;
	}

	// Generate a syscall record, maybe, via open
	fd = open(file, flags, mode);

	// Set the data
	ld->msg_type = AUDIT_MSG_SYSCALL;
	ld->laus_var_data.syscallData.code = AUDIT_open;
	ld->laus_var_data.syscallData.result = fd;
	ld->laus_var_data.syscallData.resultErrno = errno;

	// Close the file if the open succeeded
	if (fd >= 0)
		close(fd);

	// Do post-system call work
	printf5("Calling post_filter_call\n");
	if ((rc = post_filter_call(ld)) != 0)
		printf1("ERROR: post_filter_call failed - rc=%d\n", rc);

EXIT_CLEANUP:
	destroy_temp(file);

EXIT:
	printf5("Returning from test - rc=%d\n", rc);
	return rc;
}


/**********************************************************************
**  Function: test_dev
**    Tests the dev-major, dev-minor file attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Executes the open system call with a pre-determined device.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_dev(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	int fd = 0;
#if defined(__powerpc64__) || defined(__s390x__) || defined(__x86_64__)
        int flags = O_RDONLY | O_LARGEFILE;
#else
        int flags = O_RDONLY;
#endif
	int mode = 0;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Set up audit argument buffer
	printf5("Calling auditArg3\n");
	if ((rc = auditArg3(ld,
			AUDIT_ARG_PATH, strlen(test_devname), test_devname,
			AUDIT_ARG_IMMEDIATE, sizeof(flags), &flags,
			AUDIT_ARG_IMMEDIATE, sizeof(mode), &mode)) != 0) {
		printf1("ERROR: auditArg3 failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Generate a syscall record, maybe, via open
	fd = open(test_devname, flags, mode);

	// Set the data
	ld->msg_type = AUDIT_MSG_SYSCALL;
	ld->laus_var_data.syscallData.code = AUDIT_open;
	ld->laus_var_data.syscallData.result = fd;
	ld->laus_var_data.syscallData.resultErrno = errno;

	// Close the file if the open succeeded
	if (fd >= 0)
		close(fd);

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
