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
 **  FILE       : test_usermod.c
 **
 **  PURPOSE    : To test the usermod trusted program.
 **
 **  DESCRIPTION: The test_usermod() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs trusted program audit information for the
 **  "usermod" program.
 **  
 **  This function performs a series of tests on the usermod trusted
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

int test_usermod(struct audit_data* dataPtr) {

  int rc = 0;
  int test = 1;
  char* command;
  char* user;
  char* userNew;
  char* mail = NULL;
  char* mailNew = NULL;
  char* home;
  char* homeNew;
  char* group;
  char* supGroup;
  char* secondSupGroup;
  char* comment;
  char* commentNew;
  char* shell;
  char* shellNew;
  int uid;
  int uidNew;
  int gid;
  int supGid;
  int secondSupGid;
  int inactiveTime;
  int inactiveTimeNew;
  char* expire;
  char* expireNew;

  FILE* fPtr;

  if (( rc = ShadowTestSetup(1) == -1 )) {
      goto EXIT;
  }
      
  // Test 1:
  // When a user name is changed in a group.
  // The record is generated with the following commands:
  // usermod -l
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user name changed in group - user=name, olduser=name, group=name, gid=gid, by=uid
  // 
  // user = the name of the user
  // olduser = the previous name of the user
  // group = the name of the group
  // gid = the gid of the group
  // by = the uid of the user executing the command
  // 
  /**
   * Test 1 written by Michael A. Halcrow <mike@halcrow.us>
   */
 //TEST_1:
  printf("  TEST %d\n", test++);

  // Setup
  dataPtr->euid = 0;
  dataPtr->egid = 0;
  createTempUserName( &user, &uid, &home );

  // Create user
  command = mysprintf( "/usr/sbin/useradd -u %d %s", uid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );  

  createTempUserName( &userNew, &uidNew, &homeNew );

  createTempGroupName( &group, &gid );

  // Create group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", group );
    goto EXIT;
  }
  free( command );    

  createTempGroupName( &supGroup, &supGid );

  // Create supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", supGid, supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", supGroup );
    goto EXIT;
  }
  free( command );    

  createTempGroupName( &secondSupGroup, &secondSupGid );

  // Create second supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", secondSupGid, secondSupGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s]\n", secondSupGroup );
    goto EXIT;
  }
  free( command );    

  // Remove users and groups
  // Remove user
  sleep( 1 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", user );
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

  // Remove supplemantary group
  command = mysprintf( "/usr/sbin/groupdel %s", supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", supGroup );
    goto EXIT;
  }
  free( command );    

  // Remove second supplementary group
  command = mysprintf( "/usr/sbin/groupdel %s", secondSupGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", secondSupGroup );
    goto EXIT;
  }
  free( command );

  comment = "Here_from_Verona_art_thou_banished";
  commentNew = "Be_patient,_for_the_world_is_broad_and_wide";

  shell = "/bin/false";
  shellNew = "/bin/true";

  inactiveTime = -1;
  inactiveTimeNew = 1;

  expire = "2012-12-30";
  expireNew = "2012-01-01";

  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     group, gid );
    goto EXIT;
  }
  free( command );

  command = mysprintf( "/usr/sbin/groupadd -g %d %s", supGid, supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     supGroup, supGid );
    goto EXIT;
  }
  free( command );

  command = mysprintf( "/usr/sbin/useradd -g %d -u %d -d %s -G %s %s",
                       gid, uid, home, supGroup, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], homedir [%s], and initial gid [%d]\n",
	     user, uid, home, gid );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -l %s %s", userNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing name acct=%s res=success", userNew );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup
  sleep( 1 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", userNew );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", userNew );
    goto EXIT;
  }
  free( command );

  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
    goto EXIT;
  }
  free( command );

  command = mysprintf( "/usr/sbin/groupdel %s", supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", supGroup );
    goto EXIT;
  }
  free( command );
  // End Test 1

  // Test 2:
  // When a user is removed from a group.
  // The user must be a member of a group that is not in the list of groups (-G) supplied on the command to create this record.  The record is generated with the following commands:
  // usermod -G
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user removed from group - user=name, group=name, gid=gid, by=uid
  // 
  // user = the name of the user
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
  // Create initial group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     group, gid );
    goto EXIT;
  }
  free( command );

  // Create first supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", supGid, supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating supplementary group [%s] with gid [%d]\n",
	     supGroup, supGid );
    goto EXIT;
  }
  free( command );

  // Create second supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", secondSupGid, secondSupGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating second supplementary group [%s] with gid [%d]\n",
	     secondSupGroup, secondSupGid );
    goto EXIT;
  }
  free( command );  

  // Add user, including supplementary groups
  command = mysprintf( "/usr/sbin/useradd -g %d -u %d -d %s -G %s,%s %s", 
		       gid, uid, home, supGroup, secondSupGroup, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], homedir [%s], supplemenary groups [%s] and [%s], and initial gid [%d]\n",
	     user, uid, home, supGroup, secondSupGroup, gid );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -G %s %s", supGroup, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=removing user from shadow group acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );

  // Delete the initial group
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
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

  // Delete the second supplementary group
  command = mysprintf( "/usr/sbin/groupdel %s", secondSupGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", secondSupGroup );
    goto EXIT;
  }
  free( command );
  // End Test 2

  // Test 3:
  // When a user name is added to a group.
  // The user must not already be a member of a group that is in the list of groups (-G) supplied on the command to create this record.  The record is generated with the following commands:
  // usermod -l -G
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user added to group - user=name, group=name, gid=gid, by=uid
  // 
  // user = the name of the user
  // group = the name of the group
  // gid = the gid of the group
  // by = the uid of the user executing the command
  // 
  /**
   * Test 3 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_3:
  printf("  TEST %d\n", test++);
  // Create initial group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     group, gid );
    goto EXIT;
  }
  free( command );

  // Create supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", supGid, supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating supplementary group [%s] with gid [%d]\n",
	     supGroup, supGid );
    goto EXIT;
  }
  free( command );

  // Add user, including supplementary group
  command = mysprintf( "/usr/sbin/useradd -g %d -u %d -d %s %s", 
		       gid, uid, home, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], homedir [%s], and initial gid [%d]\n",
	     user, uid, home, gid );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -G %s %s", supGroup, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
   dataPtr->comm = mysprintf("usermod: op=adding user to shadow group acct=%s res=success", user );
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", userNew );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", userNew );
    goto EXIT;
  }
  free( command );

  // Delete the initial group
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
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
  // End Test 3

  // Test 4:
  // When a user's home directory is moved.
  // This record can be generated in two different ways.  The first is if the old and new home directory are on the same filesystem, the second is if the old and new home directory are on different filesystems.  The record is generated with the following commands:
  // usermod -d -m
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user home directory moved - user=name, uid=uid, home=path, oldhome=path, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // home = the home directory for the user
  // oldhome = the previous home directory for the user
  // by = the uid of the user executing the command
  // 
  /**
   * Test 4 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_4:
  printf("  TEST %d\n", test++);
  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -m %s", 
		       uid, home, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], and homedir [%s]\n",
	     user, uid, home );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -d %s_t04 -m %s", homeNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing home directory acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm = mysprintf("usermod: op=moving home directory acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // End Test 4

  // Test 5:
  // When a user's home directory owner(s) is changed.
  // The record is generated with the following commands:
  // usermod -d -m -u
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user home directory tree owner(s) changed - user=name, uid=uid, olduid=uid, gid=gid, oldgid=gid, home=path, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // olduid = the previous uid of the user
  // gid = the gid of the user
  // oldgid = the previous gid of the user
  // home = the path of the home directory
  // by = the uid of the user executing the command
  // 
  /**
   * Test 5 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_5:
  printf("  TEST %d\n", test++);
  // Add group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", 
		       gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     group, gid );
    goto EXIT;
  }
  free( command );

  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -m -g %d %s", 
		       uid, home, gid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], gid [%d], and homedir [%s]\n",
	     user, uid, gid, home );
    goto EXIT;
  }
  free( command );

  command = mysprintf( "/usr/sbin/groupadd -g %d %s", 
		       supGid, supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     supGroup, supGid );
    goto EXIT;
  }
  free( command );

  // Execution
  createTempUserName( &userNew, &uidNew, &homeNew );

  command = mysprintf( "/usr/sbin/usermod -d %s_t05 -m -u %d %s", homeNew, uidNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing uid acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm = mysprintf("usermod: op=chaning home directory acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm = mysprintf("usermod: op=moving home directory acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  dataPtr->comm = mysprintf("usermod: op=chaning home directory owner acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );

  // Delete the groups
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
    goto EXIT;
  }
  free( command );

  command = mysprintf( "/usr/sbin/groupdel %s", supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", supGroup );
    goto EXIT;
  }
  free( command );

  // End Test 5

  // Test 6:
  // When a user's uid is changed. &
  // When a user's home directory tree owner(s) is changed.
  // Although the default group gid will be changed along with the uid, this record will only be generated if the uid is changed.  The record is generated with the following commands:
  // usermod -u
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user uid changed - user=name, uid=uid, olduid=uid, by=uid
  // usermod: user home directory tree owner(s) changed - user=name, uid=uid, olduid=uid, gid=gid, oldgid=gid, home=path, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // olduid = the previous uid of the user
  // gid = the gid of the user
  // oldgid = the previous gid of the user
  // home = the path of the home directory
  // by = the uid of the user executing the command
  // 
  /**
   * Test 6 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_6:
  printf("  TEST %d\n", test++);
  // Add group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", 
		       gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     group, gid );
    goto EXIT;
  }
  free( command );

  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -m -g %d %s", 
		       uid, home, gid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], gid [%d], and homedir [%s]\n",
	     user, uid, gid, home );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -u %d %s", uidNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing uid acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
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
  // End Test 6 

  // Test 7:
  // When a user's password is changed.
  // The record is generated with the following commands:
  // usermod -p
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user password changed - user=name, uid=uid, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // by = the uid of the user executing the command
  /**
   * Test 7 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_7:
  printf("  TEST %d\n", test++);
  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d %s", 
		       uid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d]\n",
	     user, uid );
    goto EXIT;
  }
  free( command );

  // Execution
  // Produced with crypt( "eal", "42" ): 42VmxaOByKwlA
  command = mysprintf( "/usr/sbin/usermod -p 42VmxaOByKwlA %s", user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing password acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // End Test 7 

  // Test 9:
  // When a user's default group is changed.
  // The record is generated with the following commands:
  // usermod -g
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user default gid changed - user=name, uid=uid, gid=gid, oldgid=gid, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // gid = the gid of the user
  // oldgid = the previous gid of the user
  // by = the uid of the user executing the command
  // 
  /**
   * Test 9 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_9:
  printf("  TEST %d\n", test++);
  // Add group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", 
		       gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     group, gid );
    goto EXIT;
  }
  free( command );

  // Add supplementary group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", 
		       supGid, supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
	     supGroup, supGid );
    goto EXIT;
  }
  free( command );

  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -g %d %s", 
		       uid, home, gid, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], gid [%d], and homedir [%s]\n",
	     user, uid, gid, home );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -g %d %s", supGid, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing primary group acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
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

  // Delete the supplementary group
  command = mysprintf( "/usr/sbin/groupdel %s", supGroup );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting supplementary group [%s]\n", supGroup );
    goto EXIT;
  }
  free( command );
  // End Test 9 

  // Test 10:
  // When a user's comment is changed.
  // The record is generated with the following commands:
  // usermod -c
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user comment changed - user=name, uid=uid, comment='comment', oldcomment='comment', by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // comment = the comment associated with the user
  // oldcomment = the previous comment associated with the user
  // by = the uid of the user executing the command
  // 
  /**
   * Test 10 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_10:
  printf("  TEST %d\n", test++);
  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -c '%s' %s", 
		       uid, comment, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d] and comment [%s]\n",
	     user, uid, comment );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -c %s %s", commentNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing comment acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 1 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // End Test 10

  // Test 11:
  // When a user's home directory is is changed.
  // The record is generated with the following commands:
  // usermod -d
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user home directory changed - user=name, uid=uid, home=home, oldhome=oldhome, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // home = the home directory of the user
  // oldhome = the previous home directory of the user
  // by = the uid of the user executing the command
  // 
  /**
   * Test 11 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_11:
  printf("  TEST %d\n", test++);
  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s %s", 
		       uid, home, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d] and homedir [%s]\n",
	     user, uid, home );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -d %s_t11 %s", homeNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing home directory acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // End Test 11

  // Test 12:
  // When a user's shell is changed.
  // The record is generated with the following commands:
  // usermod -s
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user shell changed - user=name, uid=uid, shell=path, oldshell=path, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // shell = the path of the shell for the user
  // oldshell = the previous path of the shell for the user
  // by = the uid of the user executing the command
  // 
  /**
   * Test 12 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_12:
  printf("  TEST %d\n", test++);
  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -s %s %s", 
		       uid, home, shell, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], homedir [%s], and shell [%s]\n",
	     user, uid, home, shell );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -s %s %s", shellNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf("usermod: op=changing user shell acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // End Test 12

  // Test 13:
  // When a user's inactive time is changed.
  // The record is generated with the following commands:
  // usermod -f
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user inactive days changed - user=name, uid=uid, inactive=days, oldinactive=days, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // inactive = the number of inactive days
  // oldinactive = the previous number of inactive days
  // by = the uid of the user executing the command
  // 
  /**
   * Test 13 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_13:
  printf("  TEST %d\n", test++);
  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -f %d %s", 
		       uid, home, inactiveTime, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], homedir [%s], and inactive time [%d]\n",
	     user, uid, home, inactiveTime );
    goto EXIT;
  }
  free( command );

  // Execution
  command = mysprintf( "/usr/sbin/usermod -f %d %s", inactiveTimeNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf( "usermod: op=changing inactive days acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // End Test 13

  // Test 14:
  // When a user's expiration date is changed.
  // The record is generated with the following commands:
  // usermod -e
  // 
  // In addition to the standard audit information, the following string will be logged:
  // usermod: user expiration date changed - user=name, uid=uid, expire=date, oldexpire=date, by=uid
  // 
  // user = the name of the user
  // uid = the uid of the user
  // expire = the expiration date
  // oldexpire = the previous expiration date
  // by = the uid of the user executing the command
  // 
  /**
   * Test 14 written by Michael A. Halcrow <mike@halcrow.us>
   */
  // Setup
 //TEST_14:
  printf("  TEST %d\n", test++);
  // Add user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -e %s %s", 
		       uid, home, expire, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s] with uid [%d], homedir [%s], and expiration date [%s]\n",
	     user, uid, home, expire );
    goto EXIT;
  }
  free( command );


  // Execution
  command = mysprintf( "/usr/sbin/usermod -e %s %s", expireNew, user );
  runTrustedProgramWithoutVerify(dataPtr, command );
  free( command );

  // Check for audit records
  dataPtr->comm = mysprintf( "usermod: op=changing expiration date acct=%s res=success", user);
  verifyTrustedProgram( dataPtr );
  free( dataPtr->comm );

  // Cleanup

  // Delete the user
  sleep( 2 ); // MH: To give time for /etc/passwd to sync
  command = mysprintf( "/usr/sbin/userdel %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // End Test 14

 //EXIT_CLEANUP:

  free( user );
  free( userNew );
  free( home );
  free( homeNew );
  free( group );
  free( supGroup );
  free( secondSupGroup );
  if( mail ) {
    free( mail );
    mail = NULL;
  }
  if( mailNew ) {
    free( mailNew );
    mailNew = NULL;
  }
  
 EXIT:

  restoreFile( "/etc/default/useradd" );
  printf("Returning from test_usermod()\n");
  return rc;
}
