
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
 **  FILE       : test_useradd.c
 **
 **  PURPOSE    : To test the useradd trusted program.
 **
 **  DESCRIPTION: The test_useradd() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "useradd" program.
 **  
 **  This function performs a series of tests on the useradd trusted
 **  program.  Each subtest has its own setup, execution, and cleanup.
 **  The setup may consist of testing for the existence or non-existence
 **  of entities, the dynamic creation of entities, as well as the 
 **  definition of the expected text to be found in the audit.
 **  Each subtest is documented below to describe its own particulars.
 **
 **  HISTORY    :
 **    07/2003 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **    07/2003 Adapted by Michael A. Halcrow <mike@halcrow.us>
 **
 **********************************************************************/

#include "includes.h"
#include "trustedprograms.h"

int test_useradd(audit_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  char* user;
  char* home;  
  char* shell = "/bin/sh";
  char* group;
  char* supGroup;
  int uid;
  int gid;
  int supGid;

  FILE* fPtr;

  dataPtr->euid = 0;
  dataPtr->egid = 0;

  if (( rc = ShadowTestSetup(1) == -1 )) {
      goto EXIT;
  }

  createTempUserName( &user, &uid, &home );

  createTempGroupName( &group, &gid );

  // Create group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", group );
    goto EXIT;
  }
  free( command );    

  createTempGroupName( &supGroup, &supGid );

  // Remove group
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
    goto EXIT;
  }
  free( command );   

  // Test 1:
  // When the defaults for useradd are changed.
  // The record is generated with the following commands:
  // /usr/sbin/useradd --save-defaults -g GID -d HOMEDIR -e EXPIRE -f INACTIVE -s SHELL
  // 
  // In addition to the standard audit information, the following string will be logged
  // useradd: defaults changed - gid=GID, home=HOMEDIR, shell=SHELL, inactive=INACTIVE, expire=EXPIRE, by=ROOT 
  // 
  // gid = the gid of the default group
  // home = the path of the default home directory
  // shell = the path of the default shell
  // inactive = the # of days after a password has expired until inactivated
  // expire = the date on which an account will expire
  // by = the uid of the user executing the command
  // 

  // Setup
  // Defaults are stored in file, create backup

 //TEST_1:
  printf("  TEST %d\n", test++);

  // Create group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", group );
    goto EXIT;
  }
  free( command );    

  //Don't backup here .bak is the original we already put a test file out there
  //backupFile( "/etc/default/useradd" );

  command = mysprintf( "/usr/sbin/useradd --save-defaults -g %d -d /tmp -e 2038-01-18 -f 42 -s /bin/sh",
		       gid );
  dataPtr->comm = 
    mysprintf( "useradd: defaults changed - gid=%d, home=/tmp, shell=/bin/sh, inactive=42, expire=2038-01-18, by=0",
	       gid );

  // Execution
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup

  //Don't restore .bak has original values copy our testfile there
  //restoreFile( "/etc/default/useradd" );
  if (( rc = ShadowTestSetup(0) == -1 )) {
      goto EXIT;
  }

  // Remove group
  sleep( 2 ); // MH: The system needs some time to make certain that
	      // the entry is written to the /etc/group file.
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
    goto EXIT;
  }
  free( command );    

  free( dataPtr->comm );
  // End Test 1


  // Test 2:
  // When a user is added to a group.
  // The record is generated with the following commands:
  // useradd -G GID USER
  // 
  // In addition to the standard audit information, the following string will be logged:
  // useradd: user added to group - user=USER, uid=UID, group=GROUP, gid=GID, by=ROOT
  // 
  // user = the name of the user
  // uid = the uid of the user
  // group = the name of the group
  // gid = the gid of the group
  // by = the uid of the user executing the command
  // 
  /**
   * Test 2 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_2:
  printf("  TEST %d\n", test++);
  // Create supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", supGid, supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating supplementary group [%s] with gid [%d]\n",
	     supGroup, supGid );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/useradd -G %s -g %d -u %d %s", supGroup, supGid, uid, user );
  runTrustedProgramWithoutVerify( dataPtr, command );

  // Verify audit messages
  dataPtr->comm = 
    mysprintf("useradd: user added - user=%s, uid=%d, gid=%d, home=%s, shell=%s, by=%d", 
              user, uid, supGid, home, shell, dataPtr->euid);
  verifyTrustedProgram( dataPtr );
  free(dataPtr->comm );

  dataPtr->comm =
    mysprintf( "useradd: user added to group - user=%s, uid=%d, group=%s, gid=%d, by=%d",
               user, uid, supGroup, supGid, dataPtr->euid );
  verifyTrustedProgram( dataPtr );
  free(dataPtr->comm );

  // Cleanup
  free( command );

  // Delete the user
  sleep( 2 ); // MH: The system needs some time to make certain that
	      // the entry is written to the /etc/passwd file.
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );

  // Delete the supplementary group
  command = mysprintf( "/usr/sbin/groupdel %s", supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", supGroup );
    goto EXIT;
  }
  free( command );
  // End Test 2


  // Test 4:
  // When a user is added.
  // The record is generated with the following commands:
  // useradd 
  // 
  // In addition to the standard audit information, the following string will be logged:
  // useradd: user added - user=USER, uid=UID, gid=GID, home=HOMEDIR, shell=PATH, by=ROOT
  // 
  // user = the name of the user
  // uid = the uid of the user
  // gid = the gid of the initial group
  // home = the path of the home directory
  // shell = the path of the shell
  // by = the uid of the user executing the command
  // 
  /**
   * Test 4 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_4:
  printf("  TEST %d\n", test++);
  // Create group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     group, gid );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/useradd -u %d -g %d -d %s -s /bin/true %s", 
		       uid, gid, home, user );
  dataPtr->comm = 
    mysprintf( "useradd: user added - user=%s, uid=%d, gid=%d, home=%s, shell=/bin/true, by=%d",
	       user, uid, gid, home, dataPtr->euid );
  runTrustedProgramWithoutVerify( dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );

  // Delete the user
  sleep( 2 ); // MH: The system needs some time to make certain that
	      // the entry is written to the /etc/passwd file.
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );

  // Delete the group
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
    goto EXIT;
  }
  free( command );
  // End Test 4


  // Test 5:
  // When a user's home directory is created.
  // The record is generated with the following commands:
  // useradd -m USER
  //
  // In addition to the standard audit information, the following string will be logged:
  // useradd: user home directory created - user=USER, uid=UID, home=HOMEDIR, by=UID
  //
  // user = the name of the user
  // uid = the uid of the user
  // home = the path of the home directory
  // by = the uid of the user executing the command
  //
  /**
   * Test 5 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_5:
  printf("  TEST %d\n", test++);
  // Create group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", group );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/useradd -d %s -g %d -m -u %d %s", home, gid, uid, user );
  runTrustedProgramWithoutVerify( dataPtr, command );

  dataPtr->comm =
    mysprintf("useradd: user added - user=%s, uid=%d, gid=%d, home=%s, shell=%s, by=%d",
              user, uid, gid, home, shell, dataPtr->euid);
  verifyTrustedProgram( dataPtr );
  free(dataPtr->comm );

  dataPtr->comm = 
    mysprintf( "useradd: user home directory created - user=%s, uid=%d, home=%s, by=%d",
	       user, uid, home, dataPtr->euid );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup
  free( command );

  // Delete the user
  sleep( 2 ); // MH: The system needs some time to make certain that
	      // the entry is written to the /etc/passwd file.
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // Remove group
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
    goto EXIT;
  }
  free( command );
  // End Test 5

  // Test 6:
  // When PAM authentication fails for the user.
  // The program must be built with PAM authentication enabled.  The record is generated with the following commands:
  // useradd
  // 
  // In addition to the standard audit information, the following string will be logged:
  // useradd: PAM authentication failed - by=UID
  // 
  // by = the uid of the user executing the command
  // 
  /**
   * Test 6 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_6:
  printf("  TEST %d\n", test++);
  // Setup

  backupFile( "/etc/pam.d/useradd" );

  if( ( fPtr = fopen( "/etc/pam.d/useradd", "w" ) ) == NULL ) {
    printf( "Error opening /etc/pam.d/useradd for write w/ truncate access\n" );
    rc = -1;
    goto EXIT;
  }
  if( ( rc = fputs( "auth required pam_deny.so", fPtr ) ) == EOF ) {
    printf( "Error writing to /etc/pam.d/useradd\n" );
    goto EXIT;
  }
  if( ( rc = fclose( fPtr ) ) != 0 ) {
    printf( "Error closing file /etc/pam.d/useradd\n" );
    goto EXIT;
  }

  // Execution
  command = mysprintf( "/usr/sbin/useradd %s", user );
  dataPtr->comm = 
    mysprintf("useradd: PAM authentication failed - by=%d", dataPtr->egid );
  runTrustedProgramWithoutVerify(dataPtr, command );
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->comm );
  free( command );

  restoreFile( "/etc/pam.d/useradd" );
  // End Test 6

 //EXIT_CLEANUP:

  free(user);
  free(group);
  free(supGroup);
  free(home);

 EXIT:

  restoreFile("/etc/default/useradd");
  printf("Returning from test_useradd()\n");
  return rc;
}


