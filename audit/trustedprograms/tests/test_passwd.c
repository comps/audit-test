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
 **  FILE       : test_passwd.c
 **
 **  PURPOSE    : To test the passwd program.
 **
 **  DESCRIPTION: The test_passwd() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs program audit information for the
 **  "passwd" program.
 **  
 **  This function performs a series of tests on the passwd 
 **  program.  Each subtest has its own setup, execution, and cleanup.
 **  The setup may consist of testing for the existence or non-existence
 **  of entities, the dynamic creation of entities, as well as the 
 **  definition of the expected text to be found in the audit.
 **  Each subtest is documented below to describe its own particulars.
 **
 **  HISTORY    :
 **    08/2003 Originated by Dustin Kirkland <k1rkland@us.ibm.com>
 **
 **********************************************************************/

#include "trustedprograms.h"
#include "tempname.h"
#include <time.h>
#include <pwd.h>

int test_passwd(laus_data* dataPtr) {

  int rc = 0;
  int test = 1;

  char* user;
  char* home;
  int uid;
  char* command;
  char* filename;
  int fd;
  char* group;
  int gid;

  char* password = "eal";
  char* newpassword = "R3PgPaZ_funny_word";
  //char* newpassword = "R3PgPaZ";
//  char newpassword[9];
  //int x;
  // Produced with crypt( "eal", "42" ): 42VmxaOByKwlA
  char* encryptedpassword = "42VmxaOByKwlA";
  char* badpassword = "anything_but_eal";

  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  if (( rc = createTempUserName( &user, &uid, &home ) == -1 )) {
    printf1("Out of temp user names\n");
    goto EXIT;
  }
  if (( rc = createTempGroupName( &group, &gid ) == -1)) {
    printf1("Out of temp group names\n");
    goto EXIT;
  }

/*  srand( time(NULL) );
  for( x = 0; x < 4; x++ ) {
    newpassword[ x ] = 97+(int)(25.0*rand()/(RAND_MAX+1.0));
  } 
  for( x = 4; x < 6; x++ ) {
    newpassword[ x ] = 48+(int)(9.0*rand()/(RAND_MAX+1.0));
  }
  for( x = 6; x < 8; x++ ) {
    newpassword[ x ] = 97+(int)(25.0*rand()/(RAND_MAX+1.0));
  }
  newpassword[ 8 ] = '\0';
  printf4( "Generated password: [%s]\n", newpassword );
*/
  // Add group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s",
                       gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error creating group [%s] with gid [%d]\n",
             group, gid );
    goto EXIT;
  }
  free( command );

  // Remove laus_* users from /etc/security/opasswd
  RUNCOMMAND( "rm -f /etc/security/opasswd" );

  // Create user
  command = mysprintf( "/usr/sbin/useradd -u %d -g %d -d %s -m -p %s %s", uid, gid, home, encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // Set password
  command = mysprintf( "/usr/sbin/usermod -p %s %s", encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error setting password [%s]/[%s]\n", user, encryptedpassword );
    goto EXIT;
  }
  free( command );

  backupFile("/etc/pam.d/passwd");
  if ( ( rc = system( "cat /etc/pam.d/passwd | grep -v pam_laus.so > /tmp/passwd; mv -f /tmp/passwd /etc/pam.d/passwd; echo \"auth required pam_laus.so detach\" >> /etc/pam.d/passwd") ) == -1 ) {
    printf1( "Error modifying /etc/pam.d/passwd\n" );
    goto EXIT;
  }

  // Test 1: SUCCESS passwd
  // When a user successfully uses the passwd program.
  // The record is generated with the following commands:
  // passwd 
  //
  // In addition to the standard audit information, the following string will be logged:
  // passwd: password changed - user=USER, uid=UID, by=BY_UID
  //
  //
 //TEST_1:
  printf5("TEST %d\n", test++);
  // Setup
  // Set the date forward two years
  RUNCOMMAND( "date +%%m%%d%%H%%M%%Y > date_save.txt" );
  // MH: I had to get fancy to fix a bug with leading 0's
  RUNCOMMAND( "echo -n \"`date +%%m%%d%%H%%M`\" > notyear.txt" );
  RUNCOMMAND( "echo \"`date +%%Y`+2\" | bc > year.txt" );
  RUNCOMMAND( "date `cat notyear.txt year.txt`" );
  // Create expect script file to execute passwd session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "spawn /usr/bin/passwd\nsleep 1 \nexpect -re \"Enter current password:\" { sleep 1; exp_send \"%s\\r\\n\"} \nsleep 1 \nexpect -re \"Old Password:\" { sleep 1; exp_send \"%s\\r\\n\"} \nsleep 1\nexpect -re \"Enter new password:\" { sleep 1; exp_send \"%s\\r\\n\"} \nsleep 1\nexpect -re \"New password:\" { sleep 1; exp_send \"%s\\r\\n\"} \nsleep 1\nexpect -re \"Re-type new password:\" { sleep 1; exp_send \"%s\\r\\n\"} \nsleep 1\nexpect -re \"Re-enter new password:\" { sleep 1; exp_send \"%s\\r\\n\"} \nsleep 1\nsend_user \"\\n\"\n" , password, password, newpassword, newpassword, newpassword, newpassword);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  dataPtr->msg_ruid = uid;

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->msg_pid = NO_FORK;
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->laus_var_data.textData.data =
    mysprintf( "passwd: password changed - user=%s, uid=%d, by=%d",
               user, uid, uid );

   sleep(6); 
  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  RUNCOMMAND( "date `cat date_save.txt`" );
  RUNCOMMAND( "rm -f date_save.txt" );

  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->laus_var_data.textData.data );

  // End Test 1

  // Test 2: FAILURE passwd
  // When a user unsuccessfully uses the passwd program.
  // The record is generated with the following commands:
  // passwd (and a bad old password)
  //
  // In addition to the standard audit information, the following string will be logged:
  //  passwd: password change failed, pam error - user=dustin, uid=502, by=502
  //
  //

 //TEST_2:
  printf5("TEST %d\n", test++);
  // Setup
  RUNCOMMAND( "date +%%m%%d%%H%%M%%Y > date_save.txt" );
  RUNCOMMAND( "echo -n \"`date +%%m%%d%%H%%M`\" > notyear.txt" );
  RUNCOMMAND( "echo \"`date +%%Y`+2\" | bc > year.txt" );
  RUNCOMMAND( "date `cat notyear.txt year.txt`" );

  // Create expect script file to execute passwd session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "spawn /usr/bin/passwd\nsleep 1 \nexpect -re \"Old Password: $\" { exp_send \"%s\\r\\n\"} \nsleep 1\n", badpassword);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  dataPtr->msg_ruid = uid;

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->msg_pid = NO_FORK;
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->laus_var_data.textData.data =
    mysprintf( "passwd: password change failed, pam error - user=%s, uid=%d, by=%d",
               user, uid, uid );

  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  RUNCOMMAND( "date `cat date_save.txt`" );
  RUNCOMMAND( "rm -f date_save.txt" );
    
  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->laus_var_data.textData.data );

  // End Test 2


  // Test 3: FAILURE passwd -S -a
  // When a non-root user tries to list all passwd status.
  // The record is generated with the following commands:
  // passwd -S -a
  //
  // In addition to the standard audit information, the following string will be logged:
  //  passwd: password status display for all users denied - by=UID
  //
  //
 //TEST_3:
  printf5("TEST %d\n", test++);
  // Setup
  dataPtr->msg_ruid = uid;

  // Execution
  command = mysprintf("/usr/bin/passwd -S -a");
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->laus_var_data.textData.data =
    mysprintf( "passwd: password status display for all users denied - by=%d", uid);

  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->laus_var_data.textData.data );

  // End Test 3


  // Test 4: SUCCESS passwd -S -a
  // When root lists all passwd status.
  // The record is generated with the following commands:
  // passwd -S -a
  //
  // In addition to the standard audit information, the following string will be logged:
  // passwd: password status displayed for all users - by=0 
  //
  //
 //TEST_4:
  printf5("TEST %d\n", test++);
  // Setup
  dataPtr->msg_ruid = dataPtr->msg_euid = 0;

  // Execution
  command = mysprintf("/usr/bin/passwd -S -a");
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->laus_var_data.textData.data =
    mysprintf( "passwd: password status displayed for all users - by=%d", dataPtr->msg_euid);

  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->laus_var_data.textData.data );

  // End Test 4


  // Test 5: FAILURE passwd -k root
  // When non-root user tries to change shadow data 
  // The record is generated with the following commands:
  // passwd -k root
  //
  // In addition to the standard audit information, the following string will be logged:
  //  passwd: password change denied - user=root, uid=0, by=UID
  //
  //
 //TEST_5:
  printf5("TEST %d\n", test++);
  // Setup
  dataPtr->msg_ruid = uid;

  // Execution
  command = mysprintf("/usr/bin/passwd -k root");
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->laus_var_data.textData.data =
    mysprintf( "passwd: password change denied - user=root, uid=0, by=%d", uid);

  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->laus_var_data.textData.data );

  // End Test 5


  // Test 6: FAILURE passwd -l root
  // When non-root user tries to lock root's password
  // The record is generated with the following commands:
  // passwd -l root
  //
  // In addition to the standard audit information, the following string will be logged:
  //  passwd: password change denied - user=root, uid=0, by=UID
  //
  //
 //TEST_6:
  printf5("TEST %d\n", test++);
  // Setup
  dataPtr->msg_ruid = uid;

  // Execution
  command = mysprintf("/usr/bin/passwd -l root");
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->laus_var_data.textData.data =
    mysprintf( "passwd: password change denied - user=root, uid=0, by=%d", uid);

  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->laus_var_data.textData.data );

  // End Test 6


  // Test 7: SUCCESS passwd -l USER
  // When root locks a user's password
  // The record is generated with the following commands:
  // passwd -l USER
  //
  // In addition to the standard audit information, the following string will be logged:
  //  passwd: password changed - user=USER, uid=UID, by=0
  //
  //
 //TEST_7:
  printf5("TEST %d\n", test++);
  // Setup
  dataPtr->msg_ruid = 0;

  // Execution
  command = mysprintf("/usr/bin/passwd -l %s", user);
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->laus_var_data.textData.data =
    mysprintf( "passwd: password changed - user=%s, uid=%d, by=0", user, uid);

  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->laus_var_data.textData.data );

  // End Test 7


  // Test 8: SUCCESS passwd -S
  // When user tries to display own password information
  // The record is generated with the following commands:
  // passwd -S
  //
  // In addition to the standard audit information, the following string will be logged:
  //  passwd: password status displayed - user=USER, uid=UID, by=BY_UID
  //
  //
 //TEST_8:
  printf5("TEST %d\n", test++);
  // Setup
  dataPtr->msg_ruid = 0;

  // Execution
  command = mysprintf("/usr/bin/passwd -S");
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->laus_var_data.textData.data =
    mysprintf( "passwd: password status displayed - user=root, uid=0, by=0");

  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  free( dataPtr->laus_var_data.textData.data );

  // End Test 8





 EXIT:
  restoreFile("/etc/pam.d/passwd");
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error deleting user [%s]\n", user );
  }
  free( command );
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error deleting group [%s]\n", group );
  }
  free( command );
  printf5("Returning from test_passwd()\n");
  return rc;
}


