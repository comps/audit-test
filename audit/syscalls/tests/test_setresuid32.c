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
    **  FILE       : test_setresuid32.c
    **
    **  PURPOSE    : To test the setresuid32 library call auditing.
    **
    **  DESCRIPTION: The test_setresuid32() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "setresuid32" system call.
    **
    **  In the successful case, this function:
    **   1) Clears the audit trail
    **   2) Calls setresuid32() with euid=-1, ruid=-1, and suid=-1
    **   3) Verifies that the setresuid32 call executed successfully.
    **
    **  The successful case passes the setresuid32() parameters euid=-1,
    **  ruid=-1, suid=-1.  According to the man page, this causes no
    **  action to be taken while the syscall returns with a successful
    **  result.
    **  
    **  In the erroneous case, this function:
    **   1) Sets the euid to the test user
    **   2) Discovers an euid that will result in a failure when passed
    **      to setresuid32() by the test user
    **   3) Sets the euid to the superuser
    **   4) Clears the audit trail
    **   5) Sets the euid to the test user
    **   6) Attempts to set the euid to the unique, invalid euid
    **      determined in step (2)
    **   7) Sets the euid to the superuser
    **   8) Verifies that setresuid32 call executed erroneously.
    **      
    **  The erroneous case satisfies the condition for failure as
    **  detailed in the man page.  It sets ruid and suid to -1 to leave
    **  them unchanged, and it attempts to set euid to a unique, invalid
    **  value which will cause setresuid32() to return a failure code.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    03/04 Added exp_errno variable by D. Kent Soper <dksoper@us.ibm.com>
    **    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
    **
    **********************************************************************/

#if !defined(__PPC) && !defined(__X86_64) && !defined(__S390X)
   
   #include "syscalls.h"
   #include <unistd.h>
   
   int test_setresuid32(laus_data* dataPtr) {
     
    
     int rc = 0;
     int exp_errno = EPERM;
     int ruid, euid, suid;
     
     
     // Set the syscall-specific data
     printf5( "Setting laus_var_data.syscallData.code to %d\n", AUDIT_setresuid );
     dataPtr->laus_var_data.syscallData.code = AUDIT_setresuid;
     
     /**
      * Do as much setup work as possible right here
      */
     if( dataPtr->successCase ) {
       euid = -1;
       ruid = -1;
       suid = -1;
       printf5( "Target euid=%d, ruid=%d, and suid=%d in success case\n", euid, ruid, suid );
       dataPtr->msg_euid = 0;
       dataPtr->msg_egid = 0;
       dataPtr->msg_fsuid = 0;
       dataPtr->msg_fsgid = 0;
     } else {
       identifiers_t identifiers;
       /**
        * To test the failure case, the following conditions must apply:
        *  - I am not the superuser
        *  - The new fsuid CANNOT match any one of the following:
        *   - real group ID
        *   - effective group ID
        *   - saved set-group-ID
        *   - current value of fsuid
        */
       // Pick a nice round ID, test it, and increment it on every
       // sequential failure until we find something that works
       euid = 42;
       ruid = -1; // Just leave this unchanged
       suid = -1; // same here
   
       // su to test user
       printf5( "seteuid to %i\n", dataPtr->msg_euid );
       if( ( rc = seteuid( dataPtr->msg_euid ) ) != 0 ) {
         printf1( "Unable to seteuid to %i: errno=%i\n", 
   	       dataPtr->msg_euid, errno );
         goto EXIT; // Or possibly EXIT_CLEANUP
       }
   
       if(( rc = getIdentifiers( &identifiers ) != 0 )) {
         printf1( "Utility getIdentifiers failed\n" );
         goto EXIT;
       }
       while( euid == identifiers.ruid || euid == identifiers.euid || 
   	   euid == identifiers.suid ) {
         euid++;
       }
   
       // su to superuser
       printf5( "seteuid to root\n" );
       if ( ( rc = seteuid( 0 ) ) != 0 ) {
         printf1( "Unable to seteuid to root: errno=%i\n", errno );
         goto EXIT_CLEANUP; // Or possibly EXIT_CLEANUP
       }
       
     }
   
     // Set up audit argument buffer
     if( ( rc = auditArg3( dataPtr,
                    AUDIT_ARG_IMMEDIATE_u, sizeof( int ), &ruid,
                    AUDIT_ARG_IMMEDIATE_u, sizeof( int ), &euid,
                    AUDIT_ARG_IMMEDIATE_u, sizeof( int ), &suid ) ) != 0 ) {
       printf1( "Error setting up audit argument buffer\n" );
       goto EXIT;
     }
   
     // Do pre-system call work
     if ( (rc = preSysCall( dataPtr )) != 0 ) {
       printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
     
     dataPtr->laus_var_data.syscallData.result = syscall( __NR_setresuid32, ruid, euid, suid );
   
     // Do post-system call work
     if ( (rc = postSysCall(  dataPtr, errno, -1, exp_errno  )) != 0 ) {
       printf1("ERROR: post-syscall setup failed (%d)\n", rc);
       goto EXIT_CLEANUP;
     }
   
    EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
   
    EXIT:
     printf5( "Returning from test\n" );
     return rc;
   }

#endif
