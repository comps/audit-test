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
**  FILE   : filter_call_utils.c
**
**  PURPOSE: This file defines support routines for the filter configuration
**           tests.
**
**  DESCRIPTION: This file contains various common routines used to aid
**  the filter configuration.  The functions build into the laus_test
**  framework to provide a basis to verify that the Linux Audit System
**  accurately logs the event based upon the filter configuration.
**
**  Each function will documents its execution.
**
**
**  HISTORY:
**    08/03 originated by Tom Lendacky (toml@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include <time.h>


/**********************************************************************
**  Function: pre_filter_call
**    Performs various common operations for the filter tests
**
**   1) Fills in various "laus_data" fields via getLAUSdata.
**   2) Reloads the audit configuration files in order to activate
**      the filter statement to be tested.
**   3) Sets a beginning timestamp.
**
**********************************************************************/
int pre_filter_call(laus_data *ld) {

	int rc = 0;


	// Fill in laus_data structure
	printf5("Calling getLAUSData\n");
	if ((rc = getLAUSData(ld)) != 0) {
		printf1("ERROR: getLUASData failed - rc=%d\n", rc);
		goto EXIT;
	}

	// Reload the audit configuration
	if ((rc = reloadAudit()) != 0) {
		printf1("ERROR: reloadAudit failed - rc=%d\n", rc);
		goto EXIT;
	}

	printf5("Setting begin timestamp\n");
	ld->begin_r_time = time(NULL);

#ifndef NOSLEEP
	sleep(2);
#endif

EXIT:
	return rc;
}

/**********************************************************************
**  Function: post_filter_call
**    Performs various common operations for the filter tests
**
**   1) Sets an ending timestamp.
**   2) Resets the uid's in the event they were changed by the test.
**
**********************************************************************/
int post_filter_call(laus_data *ld) {

	int rc = 0;


#ifndef NOSLEEP
	sleep(2);
#endif

	printf5("Setting end timestamp\n");
	ld->end_r_time = time(NULL);

	// Set uid's back to root
	printf5("Resetting uid's\n");
	if ((rc = setreuid(0, 0)) != 0) {
		printf1("ERROR: setreuid failed - errno=%d %s\n", errno, strerror(errno));
		goto EXIT;
	}

EXIT:
	return rc;
}
