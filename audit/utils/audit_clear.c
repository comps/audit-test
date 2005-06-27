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
**  FILE   : audit_clear.c
**
**  PURPOSE: This file defines a function for clearing an existing
**           audit trail.  Such a function will make it easier to 
**           analyze a given audit trail and determine if the auditing
**           system accurately recorded the success or failure of the
**           system call under evaluation.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**
**********************************************************************/

#include "includes.h"

/*
** Clear audit trail
**
** BUGBUG: This is a brutal way
** to wipe the logs.
*/
int clearAuditTrail() {

  int rc = 0;

  printf4("clearAuditTrail\n");
  system("rm -f /var/log/audit.d/*");
  system("rm -f /var/log/audit");

  return rc;
}


/**
 * Stop the auditing daemon, clear the audit trail, and start the
 * audit daemon.
 */
/* public */ int
stopClearStartAudit()
{
	int rc;
	if( ( rc = stopAudit() ) < 0 ) {
		printf1( "Error stopping audit daemon: [%d]\n", rc );
		goto EXIT;
	}
	if( ( rc = clearAuditTrail() ) < 0 ) {
		printf1( "Error clearing the audit trail: [%d]\n", rc );
		goto EXIT;
	}
	if( ( rc = startAudit() ) < 0 ) {
		printf1( "Error starting the audit daemon: [%d]\n", rc );
		goto EXIT;
	}
	
 EXIT:
	return rc;
}
