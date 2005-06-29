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
**  FILE   : trusted_program.c
**
**  PURPOSE: This file defines a utility functions that run trusted
**           programs and verify the auditing results.
**
**
**  HISTORY:
**    07/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
**    07/03 re-implemented by Daniel H. Jones (danjones@us.ibm.com)
**
**********************************************************************/

#include "includes.h"
#include "trustedprograms.h"
#include "logoptions.h"
#include <sys/wait.h>

int runTrustedProgramAndVerify(laus_data* dataPtr, char* command) {

  int rc  = 0;

  if( ( rc = runTrustedProgramWithoutVerify( dataPtr, command ) ) != 0 ) {
    printf1( "Error during trusted program run\n" );
    goto EXIT;
  }

  verifyTrustedProgram( dataPtr );

EXIT:
  return rc;

}

int runTrustedProgramWithoutVerify( laus_data* dataPtr, char* command ) {
  int rc = 0;
  int pid = 0;

  if ((rc = preTrustedProgram(dataPtr) != 0)) {
    printf1("ERROR: preTrustedProgram failed\n");
    goto EXIT;
  }

   printf5("Run command '%s'\n", command);
  // Get pid of exec'd trusted program
   if ( dataPtr->msg_pid != NO_FORK ) {
     pid = run(command);
   } else {
     system( command );
     dataPtr->msg_pid = NO_PID_CHECK;
   }

   if ( dataPtr->msg_pid != NO_PID_CHECK ) {
       dataPtr->msg_pid = pid;
   }
       
  if(( rc = postTrustedProgram( dataPtr ) != 0 )) {
    printf1( "ERROR: postTrustedProgram failed\n" );
    goto EXIT;
  }

EXIT:
  return rc;
}

int verifyTrustedProgram(laus_data *dataPtr) {
    int rc = 0;

    rc = audit_verify_log(dataPtr, logOptions[LOGOPTION_INDEX_ALL]);

    printf2("Verify record\n");
    if (rc > 0) {
        pass_testcases++;
        printf2("AUDIT PASS ");
    } else {
        fail_testcases++;
        debug_expected(dataPtr);
        printf2("AUDIT FAIL ");
    }
    printf2prime(
        ": '%s' [logSuccess=1, logFailure=1, successCase=%x]\n",
        dataPtr->testName, dataPtr->successCase);

    return rc;
}

#undef system
int SystemX(char *command) {
       int rc;

       printf5("Run command: %s\n", command);
       rc = system(command);
       if (rc != -1 && WEXITSTATUS(rc) > 0) {
               printf5("command '%s' returns %d\n", command, WEXITSTATUS(rc));
               rc = -1;
       }
       return rc;
}
