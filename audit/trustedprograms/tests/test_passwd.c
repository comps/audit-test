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

#include "includes.h"
#include "trustedprograms.h"
#include "tempname.h"
#include <time.h>
#include <pwd.h>

int test_passwd(struct audit_data* dataPtr) {

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
  // Produced with crypt( "eal", "42" ): 42VmxaOByKwlA
  char* encryptedpassword = "42VmxaOByKwlA";
  char* newpassword = "R3PgPaZ_funny_word";
  char* badpassword = "anything_but_eal";

  dataPtr->euid = 0;
  dataPtr->egid = 0;
  if (( rc = createTempUserName( &user, &uid, &home ) == -1 )) {
    printf("Out of temp user names\n");
    goto EXIT;
  }
  if (( rc = createTempGroupName( &group, &gid ) == -1)) {
    printf("Out of temp group names\n");
    goto EXIT;
  }

  // Add group
  command = mysprintf( "/usr/sbin/groupadd -g %d %s", gid, group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating group [%s] with gid [%d]\n",
             group, gid );
    goto EXIT;
  }
  free( command );

  // Create user
  command = mysprintf( "/usr/sbin/useradd -u %d -g %d -d %s -m -p %s %s", uid, gid, home, encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // Set password
  command = mysprintf( "/usr/sbin/usermod -p %s %s", encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error setting password [%s]/[%s]\n", user, encryptedpassword );
    goto EXIT;
  }
  free( command );

  /*  We don't have pam_laus 
  backupFile("/etc/pam.d/passwd");
  if ( ( rc = system( "cat /etc/pam.d/passwd | grep -v pam_laus.so > /tmp/passwd; mv -f /tmp/passwd /etc/pam.d/passwd; echo \"auth required pam_laus.so detach\" >> /etc/pam.d/passwd") ) == -1 ) {
    printf( "Error modifying /etc/pam.d/passwd\n" );
    goto EXIT;
  } */

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
  printf("TEST %d\n", test++);
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
  command = mysprintf( "spawn /usr/bin/passwd\n"
"sleep 1 \n"
"expect -re \"Enter current password:\" { sleep 1; exp_send \"%s\\r\\n\"} \n"
"sleep 1 \n"
"expect -re \"(current) UNIX password:\" { sleep 1; exp_send \"%s\\r\\n\"} \n"
"sleep 1\n"
"expect -re \"New UNIX password:\" { sleep 1; exp_send \"%s\\r\\n\"} \n"
"sleep 1\n"
"expect -re \"Retype new UNIX password:\" { sleep 1; exp_send \"%s\\r\\n\"} \n"
"sleep 1\n"
"send_user \"\\n\"\n" , password, password, newpassword, newpassword, newpassword, newpassword);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  dataPtr->uid = uid;

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->pid = NO_FORK;
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->comm = mysprintf( "PAM chauthtok: user=%s exe=\"/usr/bin/passwd\" (hostname=?, addr=?, terminal=? result=Success)", user );

   sleep(6); 
  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  RUNCOMMAND( "date `cat date_save.txt`" );
  RUNCOMMAND( "rm -f date_save.txt" );

  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->comm );

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
  printf("TEST %d\n", test++);
  // Setup
  RUNCOMMAND( "date +%%m%%d%%H%%M%%Y > date_save.txt" );
  RUNCOMMAND( "echo -n \"`date +%%m%%d%%H%%M`\" > notyear.txt" );
  RUNCOMMAND( "echo \"`date +%%Y`+2\" | bc > year.txt" );
  RUNCOMMAND( "date `cat notyear.txt year.txt`" );

  // Create expect script file to execute passwd session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "spawn /usr/bin/passwd\n"
"sleep 1 \n"
"expect -re \"(current) UNIX password: $\" { exp_send \"%s\\r\\n\"} \n"
"sleep 1\n", badpassword);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  dataPtr->uid = uid;

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->pid = NO_FORK;
  runTrustedProgramWithoutVerify( dataPtr, command );
  free( command );

  // Set up dataPtr to compare against audit record
  dataPtr->comm = mysprintf( "PAM chauthtok: user=%s exe=\"/usr/bin/passwd\" (hostname=?, addr=?, terminal=? result=Authentication token manipulation error)", user);

  // Check for audit record
  verifyTrustedProgram( dataPtr );

  // Cleanup
  RUNCOMMAND( "date `cat date_save.txt`" );
  RUNCOMMAND( "rm -f date_save.txt" );
    
  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->comm );

  // End Test 2


 EXIT:
  restoreFile("/etc/pam.d/passwd");
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
  }
  free( command );
  command = mysprintf( "/usr/sbin/groupdel %s", group );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting group [%s]\n", group );
  }
  free( command );
  printf("Returning from test_passwd()\n");
  return rc;
}


