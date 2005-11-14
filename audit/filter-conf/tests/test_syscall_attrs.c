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
**  FILE       : test_syscall_attrs.c
**
**  PURPOSE    : Used to test the audit filter configuration.
**
**  DESCRIPTION: This file contains various routines used to test
**  the filter configuration support of syscall attributes.  The
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

#include <sys/socket.h>
#include <linux/net.h>


/**********************************************************************
**  Function: test_syscall_argN
**    Tests the argN system call support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Executes the setreuid system call with pre-determined uid's.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_syscall_argN(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	uid_t r_uid = uid;
	uid_t e_uid = 0;



	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Set up audit argument buffer
	printf5("Calling auditArg2\n");
	if ((rc = auditArg2(ld,
			AUDIT_ARG_IMMEDIATE, sizeof(r_uid), &r_uid,
			AUDIT_ARG_IMMEDIATE, sizeof(e_uid), &e_uid)) != 0) {
		printf1("ERROR: auditArg2 failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Generate a syscall record, maybe, via setreuid
//	rc = setreuid(r_uid, e_uid);
	rc = syscall(__NR_setreuid, r_uid, e_uid);

	// Set the data
	ld->msg_type = AUDIT_MSG_SYSCALL;
	ld->laus_var_data.syscallData.code = AUDIT_setreuid;
	ld->msg_ruid = r_uid;
	ld->msg_euid = e_uid;
	ld->msg_suid = e_uid;
	ld->msg_fsuid = e_uid;
	ld->laus_var_data.syscallData.result = rc;
	ld->laus_var_data.syscallData.resultErrno = errno;

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

#if !defined(__x86_64__) && !defined(__ia64__)
//no syscalls have minor numbers other than 0
/**********************************************************************
**  Function: test_syscall_minor
**    Tests the syscall-minor system call support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Executes the socket system call with a pre-determined parameters.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_syscall_minor(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	int sd = 0;
	int family = PF_INET;
	int type = SOCK_STREAM;
	int proto = 0;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Set up audit argument buffer
	printf5("Calling auditArg3\n");
	if ((rc = auditArg3(ld,
			AUDIT_ARG_IMMEDIATE, sizeof(family), &family,
			AUDIT_ARG_IMMEDIATE, sizeof(type), &type,
			AUDIT_ARG_IMMEDIATE, sizeof(proto), &proto)) != 0) {
		printf1("ERROR: auditArg3 failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Generate a syscall record, maybe, via socket
	sd = socket(PF_INET, SOCK_STREAM, 0);

	// Set the data
	ld->msg_type = AUDIT_MSG_SYSCALL;

	ld->laus_var_data.syscallData.code = __NR_socketcall;
	ld->laus_var_data.syscallData.minor = SYS_SOCKET;

	ld->laus_var_data.syscallData.result = sd;
	ld->laus_var_data.syscallData.resultErrno = errno;

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
#endif
