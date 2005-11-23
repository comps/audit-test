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


  // Test 1:
  // When a user is removed from a group.
  // The record is generated with the following commands:
  // userdel
  // 
  // In addition to the standard audit information, the following string will be logged:
  // userdel: user removed from group - user=user, uid=uid, group=group, gid=gid, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // group = the name of the group
  // gid = the gid of the group
  // by = the uid of the user executing the command
  /**
   * Test 1 written by Michael A. Halcrow <mike@halcrow.us>
   */

 //TEST_1:
  printf("  TEST %d\n", test++);

  // Setup
  dataPtr->euid = 0;
  dataPtr->egid = 0;
  createTempUserName( &user, &uid, &home );
  createTempGroupName( &group, &gid );

  // Create the auxiliary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating new group [%s] with gid [%d]\n", group, gid );
    goto EXIT;
  }
  free( command );

  // Create the user and add him to the group
  command = mysprintf( "/usr/sbin/useradd -u %d -g users -G %s %s", uid, group, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user account [%s] with uid [%d] with initial group [users] and auxiliary group [%s]\n", 
	     user, uid, group );
    goto EXIT;
  }
  free( command );

  // Execution
  sleep( 1 ); // MH: To give time for /etc/passwd to be sync'd
  command = mysprintf( "/usr/sbin/userdel %s", user );
  runTrustedProgramWithoutVerify(dataPtr, command );

  dataPtr->comm = 
    mysprintf("userdel: user removed from group - user=%s, uid=%d, group=%s, gid=%d, by=%d",
	      user, uid, group, gid, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm = mysprintf("userdel: user deleted - user=%s, uid=%d, by=%d",
              user, uid, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup
  free( command );
  sleep( 1 );
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
    goto EXIT;
  }
  free( command );
  //
  // End Test 1

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
    mysprintf("userdel: user deleted - user=%s, uid=%d, by=%d",
	      user, uid, dataPtr->egid );
  runTrustedProgramWithoutVerify(dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );
  // End Test 2

  // Test 3:
  // When a user deletion command is run.
  // If a user deletion command is defined this record will be generated.  The user deletion command can be defined through the login defs file, /etc/login.defs (default).  The record is generated with the following commands:
  // userdel
  // 
  // In addition to the standard audit information, the following string will be logged:
  // userdel: running user deletion command - user=user, uid=uid, cmd=path, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // cmd = the path of the user deletion command
  // by = the uid of the user executing the command
  // 
  /**
   * Test 3 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_3:
  printf("  TEST %d\n", test++);
  // Setup
  // Create the user
  sleep( 1 );
  command = mysprintf( "/usr/sbin/useradd -u %d %s", uid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user account [%s] with uid [%d]\n", 
	     user, uid );
    goto EXIT;
  } else {
    rc = 0;
  }
  free( command );

  // Execution
  sleep( 1 ); // MH: To give time for /etc/passwd to be sync'd
  command = mysprintf( "/usr/sbin/userdel %s", user );
  dataPtr->comm =
    mysprintf("userdel: running user pre deletion command - user=%s, uid=%d, cmd=/usr/sbin/userdel-pre.local, by=%d",
	      user, uid, dataPtr->egid );
  //testing the user defined deletion command runs as a different pid
  dataPtr->pid = NO_PID_CHECK;
  printf("pid passed is: [%d]\n", dataPtr->pid);
  runTrustedProgramWithoutVerify(dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );

  sleep( 1 );
  // End Test 3


  // Test 4:
  // When a user deletion command is run.
  // If a user deletion command is defined this record will be generated.  The user deletion command can be defined through the login defs file, /etc/login.defs (default).  The record is generated with the following commands:
  // userdel
  // 
  // In addition to the standard audit information, the following string will be logged:
  // userdel: running user deletion command - user=user, uid=uid, cmd=path, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // cmd = the path of the user deletion command
  // by = the uid of the user executing the command
  // 
  /**
   * Test 4 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_4:
  printf("  TEST %d\n", test++);
  // Setup
  // Create the user
  sleep( 1 );
  command = mysprintf( "/usr/sbin/useradd -u %d %s", uid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user account [%s] with uid [%d]\n", 
	     user, uid );
    goto EXIT;
  } else {
    rc = 0;
  }
  free( command );

  // Execution
  sleep( 2 ); // MH: To give time for /etc/passwd to be sync'd
  command = mysprintf( "/usr/sbin/userdel %s", user );
  dataPtr->comm = 
    mysprintf("userdel: running user post deletion command - user=%s, uid=%d, cmd=/usr/sbin/userdel-post.local, by=%d",
	      user, uid, dataPtr->egid );
  //testing the user defined deletion command runs as a different pid
  dataPtr->pid = NO_PID_CHECK;
  printf("pid passed is: [%d]\n", dataPtr->pid);
  runTrustedProgramWithoutVerify(dataPtr, command );
  sleep( 2 );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );

  // End Test 4


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

  dataPtr->comm = 
    mysprintf( "userdel: user mail file deleted - user=%s, uid=%d, mailfile=/var/mail/%s, by=%d",
	       user, uid, user, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm =
    mysprintf("userdel: user deleted - user=%s, uid=%d, by=%d",
              user, uid, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm =
    mysprintf("userdel: user home directory deleted - user=%s, uid=%d, home=%s, by=%d",
              user, uid, home, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup
  free( command );
  // End Test 5


  // Test 6:
  // When a user's mail file is forcefully deleted.
  // The mail file name is created based on the login definitions file, /etc/login.defs, or how the program was built, configure.  The record is generated with the following commands:
  // userdel -r -f
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
   * Test 6 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_6:
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
  command = mysprintf( "/usr/sbin/userdel -r -f %s", user );
  runTrustedProgramWithoutVerify( dataPtr, command );

  dataPtr->comm =
    mysprintf( "userdel: user mail file deleted - user=%s, uid=%d, mailfile=/var/mail/%s, by=%d",
               user, uid, user, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm =
    mysprintf("userdel: user deleted - user=%s, uid=%d, by=%d",
              user, uid, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm =
    mysprintf("userdel: user home directory deleted - user=%s, uid=%d, home=%s, by=%d",
              user, uid, home, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup
  free( command );
  // End Test 6


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

  // Create mail file
  RUNCOMMANDORDIE("touch /var/mail/%s; chown %s /var/mail/%s", user, user, user);

  // Execution
  sleep( 1 ); // MH: To give time for /etc/passwd to be sync'd
  command = mysprintf( "/usr/sbin/userdel -r -f %s", user );
  runTrustedProgramWithoutVerify( dataPtr, command );

  dataPtr->comm =
    mysprintf("userdel: user home directory deleted - user=%s, uid=%d, home=%s, by=%d",
              user, uid, home, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm =
    mysprintf( "userdel: user mail file deleted - user=%s, uid=%d, mailfile=/var/mail/%s, by=%d",
               user, uid, user, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm =
    mysprintf("userdel: user deleted - user=%s, uid=%d, by=%d",
              user, uid, dataPtr->egid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup
  free( command );
  // End Test 7


  // Test 8:
  // When PAM authentication fails for the user.
  // The program must be built with PAM authentication enabled.  The record is generated with the following commands:
  // userdel
  // 
  // In addition to the standard audit information, the following string will be logged:
  // userdel: PAM authentication failed - by=uid
  // 
  // by = the uid of the user executing the command
  // 
  /**
   * Test 8 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_8:
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

  backupFile( "/etc/pam.d/shadow" );

  if( ( fPtr = fopen( "/etc/pam.d/shadow", "w" ) ) == NULL ) {
    printf( "Error opening /etc/pam.d/shadow for write w/ truncate access\n" );
    rc = -1;
    goto EXIT;
  }
  if( ( rc = fputs( "auth required pam_deny.so", fPtr ) ) == EOF ) {
    printf( "Error writing to /etc/pam.d/shadow\n" );
    goto EXIT;
  }
  if( ( rc = fclose( fPtr ) ) != 0 ) {
    printf( "Error closing file /etc/pam.d/shadow\n" );
    goto EXIT;
  }

  // Execution
  sleep( 1 ); // MH: To give time for /etc/passwd to be sync'd
  command = mysprintf( "/usr/sbin/userdel %s", user );
  dataPtr->comm =
    mysprintf("userdel: PAM authentication failed - by=%d", dataPtr->egid );
  runTrustedProgramWithoutVerify(dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );

  restoreFile( "/etc/pam.d/shadow" );

  command = mysprintf( "/usr/sbin/userdel %s", user );
  system( command );
  free( command );

  // End Test 8

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


