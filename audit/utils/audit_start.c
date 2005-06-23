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
**  FILE   : audit_start.c
**
**  PURPOSE: This file defines a function that will start the auditing
**           system that is going to be tested.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**    11/03 Modified to use /etc/init.d/audit script by Michael A. Halcrow
**          <mike@halcrow.us>
**
**********************************************************************/

#include "utils.h"

#include "laus.h"

/*
** Start auditing system calls
**
*/
int startAudit() {

  int rc = 0;
  printf4("startAudit\n");

  // Clear the audit logs
  // MH: Leave it up to the tests to do this
//  if( rc = clearAuditTrail() ) {
//    printf1( "ERROR: Problem occurred while clearing the audit trail\n" );
//    goto EXIT_ERROR;
//  }

  if ( ( rc = system( "/etc/init.d/audit start" ) ) == -1 ) {
      printf1("ERROR: Unable to start auditd; errno=%i\n", errno);
      goto EXIT_ERROR;
  }

  sleep(2);

#if 0
  if ( (rc = laus_open(NULL)) < 0 ) {
      printf1("ERROR: Unable to open auditing errno=%d %s\n", errno, laus_strerror(errno));
      goto EXIT_ERROR;
  }

  if ( (rc = laus_attach()) < 0 ) {
      printf1("ERROR: Unable to establish auditing for the current process errno=%d %s\n", errno, laus_strerror(errno));
      goto EXIT_ERROR;
  }

  if ( (rc = laus_setauditid()) < 0 ) {
      printf1("ERROR: Unable to set the audit id for the current process errno=%d %s\n", errno, laus_strerror(errno));
      goto EXIT_ERROR;
  }

  if ( (rc = laus_setsession(login_uid, NULL, NULL, ttyname(0))) < 0 ) {
      printf1("ERROR: Unable to set the session id for the current process errno=%d %s\n", errno, laus_strerror(errno));
      goto EXIT_ERROR;
  }
#endif

EXIT_ERROR:
  return rc;
}
