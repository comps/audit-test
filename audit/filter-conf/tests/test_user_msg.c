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
**  FILE       : test_user_msg.c
**
**  PURPOSE    : Used to test the audit filter configuration.
**
**  DESCRIPTION: This file contains various routines used to test
**  the filter configuration support of user messages.  The
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


/**********************************************************************
**  Function: test_user_msg
**    Tests the user message support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Executes the system call with a pre-determined userland command.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_user_msg(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	pid_t pid = 0;


	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Run the command (forks and execves)
	printf5("Preparing to run '%s' command\n", GROUP_ADD_CMD);
	pid = run(GROUP_ADD_CMD);
	// Use new PID
	printf5("Command successfully run - pid=%d\n", pid);
	ld->msg_pid = pid;

	// Set the data
	ld->msg_type = AUDIT_MSG_USER;
	ld->laus_var_data.textData.data = mysprintf(GROUP_ADD_MSG, ld->msg_ruid);

	// Remove the group
	system(GROUP_DEL_CMD);

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
