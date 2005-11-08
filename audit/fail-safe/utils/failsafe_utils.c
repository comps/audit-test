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
**  FILE   : failsafe_utils.c
**
**  PURPOSE: This file defines support routines for the failsafe tests.
**
**  DESCRIPTION: This file contains various common routines used to aid
**  the failsafe tests.  The functions build into the laus_test
**  framework to provide a basis to verify that the Linux Audit System
**  performs as designed.
**
**  Each function will documents its execution.
**
**
**  HISTORY:
**    09/03 originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include <time.h>


/**********************************************************************
**  Function: pre_failsafe_call
**    Performs various common operations for the failsafe tests
**
**   1) ...
**
**********************************************************************/
int pre_failsafe_call(laus_data *ld) {

	int rc = 0;


	// Fill in laus_data structure
	printf5("Calling getLAUSData\n");
	if ((rc = getLAUSData(ld)) != 0) {
		printf1("ERROR: getLUASData failed - rc=%d\n", rc);
		goto EXIT;
	}

	// In order to use smaller pre-allocated log files:
	//   - Stop auditd
	//   - Erase log files
	//   - Start auditd (now using new config files)
	if (audit_stop() < 0) {
	    printf1( "Error stopping audit daemon\n");
	}
	if (audit_clear_logs() < 0) {
	    printf1( "Error clearing the audit trail\n");
	}
	if (audit_start() < 0 ) {
	    printf1( "Error starting the audit daemon\n");
	}

EXIT:
	return rc;
}

/**********************************************************************
**  Function: post_failsafe_call
**    Performs various common operations for the failsafe tests
**
**   1) ...
**   2) Resets the uid's in the event they were changed by the test.
**
**********************************************************************/
int post_failsafe_call(laus_data *ld) {

	int rc = 0;


	// Set uid's back to root
	printf5("Resetting uid's\n");
	if ((rc = setreuid(0, 0)) != 0) {
		printf1("ERROR: setreuid failed - errno=%d %s\n", errno, strerror(errno));
		goto EXIT;
	}

EXIT:
	return rc;
}
