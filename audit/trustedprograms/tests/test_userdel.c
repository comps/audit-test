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
 **  FILE       : test_userdel.c
 **
 **  PURPOSE    : To test the userdel trusted program.
 **
 **  DESCRIPTION: The test_userdel() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "userdel" program.
 **  
 **  This function performs a series of tests on the userdel trusted
 **  program.  Each subtest has its own setup, execution, and cleanup.
 **  The setup may consist of testing for the existence or non-existence
 **  of entities, the dynamic creation of entities, as well as the 
 **  definition of the expected text to be found in the audit.
 **  Each subtest is documented below to describe its own particulars.
 **
 **  HISTORY    :
 **    07/2003 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **    07/2003 Furthered by Michael A. Halcrow <mike@halcrow.us>
 **
 **********************************************************************/

#include "includes.h"
#include "trustedprograms.h"

int test_userdel(struct audit_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  char* user;
  char* group;
  char* home;
  int uid;
  int gid;
  FILE* fPtr;
  // Setup

  dataPtr->euid = 0;
  dataPtr->egid = 0;

  if (( rc = ShadowTestSetup(1) == -1 )) {
      goto EXIT;
  }

  createTempUserName( &user, &uid, &home );
  createTempGroupName( &group, &gid );

  // Test 2:
  // When a user is deleted.
  // The record is generated with the following commands:
  // userdel
  // 
  // In addition to the standard audit information, the following string will be logged:
  // userdel: user deleted - user=user, uid=uid, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // by = the uid of the user executing the command
  // 
  /**
   * Test 2 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_2:
  printf("  TEST %d\n", test++);
  // Setup

  // Create the user
  sleep( 1 );
  command = mysprintf( "/usr/sbin/useradd -u %d %s", uid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user account [%s] with uid [%d]\n", 
	     user, uid );
    goto EXIT;
  }
  free( command );

  // Execution
  sleep( 1 ); // MH: To give time for /etc/passwd to be sync'd
  command = mysprintf( "/usr/sbin/userdel %s", user );
  dataPtr->comm = 
    mysprintf("userdel: op=deleting user entries acct=%s res=success", user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );
  // End Test 2

  // Test 5:
  // When a user's mail file is deleted.

  // Test 5:
  // When a user's mail file is deleted.
  // Various situations can cause this record to not be generated, such as the owner of the file being different than the user being deleted, the file not existing or the unlink call failing.  The mail file name is created based on the login definitions file, /etc/login.defs, or how the program was built, configure.  The record is generated with the following commands:
  // userdel -r
  // 
  // In addition to the standard audit information, the following string will be logged:
  // userdel: user mail file deleted - user=user, uid=uid, mailfile=path, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // mailfile = the path of the home directory
  // by = the uid of the user executing the command
  // 
  /**
   * Test 5 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_5:
  printf("  TEST %d\n", test++); 
  // Add user
  sleep( 1 );
  RUNCOMMANDORDIE( "/usr/sbin/useradd -u %d -d %s -m %s", 
		   uid, home, user );

 // Create mail file
  sleep( 1 );
  RUNCOMMANDORDIE("touch /var/mail/%s; chown %s /var/mail/%s", user, user, user);


  // Execution
  sleep( 1 ); // MH: To give time for /etc/passwd to be sync'd
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  runTrustedProgramWithoutVerify( dataPtr, command );
  sleep( 1 ); 

  dataPtr->comm = mysprintf("userdel: op=deleting user entries acct=%s res=success", user );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // XXX failed?  I don't think so
  dataPtr->comm = mysprintf("userdel: op=deleting mail file acct=%s res=failed", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm = mysprintf("userdel: op=deleting home directory acct=%s res=success", user );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup
  free( command );
  // End Test 5


  // Test 7:
  // When a user's home directory is deleted.
  // The record is generated with the following commands:
  // userdel -r
  // 
  // In addition to the standard audit information, the following string will be logged:
  // userdel: user home directory deleted - user=user, uid=uid, home=path, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // home = the path of the home directory
  // by = the uid of the user executing the command
  // 
  /**
   * Test 7 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_7:
  printf("  TEST %d\n", test++);

  // Add user
  sleep( 1 );
  RUNCOMMANDORDIE( "/usr/sbin/useradd -u %d -d %s -m %s",
                   uid, home, user );

  // Execution
  sleep( 1 ); // MH: To give time for /etc/passwd to be sync'd
  command = mysprintf( "/usr/sbin/userdel -r -f %s", user );
  runTrustedProgramWithoutVerify( dataPtr, command );

  // Check
  dataPtr->comm = mysprintf("userdel: op=deleting user entries acct=%s res=success", user );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm = mysprintf("userdel: op=deleting home directory acct=%s res=success", user );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup
  free( command );
  // End Test 7


 EXIT_CLEANUP:

  free( user );
  free( home );
  free( group );
  rc = 0;

 EXIT:

  restoreFile( "/etc/default/useradd" );
  printf("Returning from test_userdel()\n");
  return rc;
}


