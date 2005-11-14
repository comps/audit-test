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
    **  FILE       : test_capset.c
    **
    **  PURPOSE    : To test the capset library call auditing.
    **
    **  DESCRIPTION: The test_capset() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "capset" system call.
    **
    **  In the successful case, this function:
    **   1) Gets the current capabilities
    **   2) Executes the capset syscall with the results of the
    **      previously executed capget syscall
    **   3) Verifies that the result was successful.
    **
    **  The successful case does not change any of the capabilities.
    **  Since the arguments are valid and since none of the capabilities
    **  are changed, there is no possible error condition for the call
    **  (according to the man page for capset).  We can thus expect a
    **  success result from capset in the successful case.
    **  
    **  In the erroneous case, this function:
    **   1) Gets the current capabilities
    **   2) Bitwise negates the data->permitted value
    **   4) Executes the capset syscall with the modified permitted value
    **   6) Verifies that the result was erroneous.
    **      
    **  The erroneous case attempts to change the permitted capability
    **  set, which, according to the capset man page, causes an EPERM
    **  result due to the fact that we try to add a capability to the
    **  Permitted set.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **
    **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <linux/capability.h>
extern int capget(cap_user_header_t header, const cap_user_data_t data);

int test_capset(struct audit_data *context)
{

    int rc = 0;
    int exp_errno = EPERM;
    __u64 version, pid;

    cap_user_header_t header =
	(cap_user_header_t) malloc(sizeof(struct __user_cap_header_struct));
    cap_user_data_t data =
	(cap_user_data_t) malloc(sizeof(struct __user_cap_data_struct));

     /**
      * Do as much setup work as possible right here
      */
    version = header->version = _LINUX_CAPABILITY_VERSION;
    pid = header->pid = 0;
    if (capget(header, data) == -1) {
	printf1("Error calling capget: errno=%i\n", errno);
	goto EXIT;
    }

    /*
     ** Enable capset for this process
     */
    data->effective = data->effective | CAP_SETPCAP;
    if ((rc = syscall(__NR_capset, header, data)) == -1) {
	printf1("Error initializing effective capabilities\n");
	goto EXIT_CLEANUP;
    }

    if (!context->success) {
	data->permitted = ~data->permitted;
    }
    // Set up audit argument buffer
    if ((rc = auditArg5(context,
			AUDIT_ARG_IMMEDIATE, sizeof(__u64), &version,
			AUDIT_ARG_IMMEDIATE, sizeof(__u64), &pid,
			AUDIT_ARG_POINTER, sizeof(__u32), &data->effective,
			AUDIT_ARG_POINTER, sizeof(__u32), &data->inheritable,
			AUDIT_ARG_POINTER, sizeof(__u32),
			&data->permitted)) != 0) {
	printf1("Error setting up audit argument buffer\n");
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }
    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    context->u.syscall.exit = syscall(__NR_capset, header, data);

    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:

EXIT:
    free(header);
    free(data);
    printf5("Returning from test\n");
    return rc;
}
