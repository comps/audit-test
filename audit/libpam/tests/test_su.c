
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
 **  FILE       : test_su.c
 **
 **  PURPOSE    : To test the su PAM program.
 **
 **  DESCRIPTION: The test_su() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs PAM program audit information for the
 **  "su" program.
 **  
 **  This function performs a series of tests on the su PAM
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
#include "libpam.h"
#include "tempname.h"
#include <time.h>
#include <pwd.h>

int test_su(audit_data* dataPtr) {

  int rc = 0;
  int test = 1;

  char* user;
  char* home;
  int uid;
  char* command;
  char* filename;
  char* pts_filename;
  char* pts_filename2;
  //char* dummy;
  int pts;
  int fd;
  FILE* fPtr;

  // Produced with crypt( "eal", "42" ): 42VmxaOByKwlA
  char* password = "eal";
  char* encryptedpassword = "42VmxaOByKwlA";
  char* badpassword = "anything_but_eal";

  dataPtr->euid = 0;
  dataPtr->egid = 0;
  if (( rc = createTempUserName( &user, &uid, &home ) == -1 )) {
    printf("Out of temp user names\n");
    goto EXIT;
  }
  // Create user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -g wheel -G wheel -m -p %s %s", uid, home, encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );

  // Test 1: SUCCESS su
  // When a user successfully uses the su program.
  // The record is generated with the following commands:
  // su
  //
  // In addition to the standard audit information, the following string will be logged:
  // PAM authentication: user=USERNAME (hostname=?, addr=?, terminal=pts/PTS)
  // PAM accounting: user=USERNAME (hostname=?, addr=?, terminal=pts/PTS)
  //
  //
 //TEST_1:
  printf("TEST %d\n", test++);
  // Setup
  // Create expect script file to execute su session
  createTempFileName(&pts_filename);
  createTempFileName(&pts_filename2);
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
//command = mysprintf( "spawn /bin/su - %s\nsleep 1 \nexpect -re \"Password: $\" { sleep 1; exp_send \"%s\\r\\n\"} \nsleep 1 \nexpect -re \" $\" { sleep 1; exp_send \"/usr/bin/tty > %s\\r\\n\"} \nsleep 1 \nexpect \" $\" { sleep 1; exp_send \"exit\\r\"; send_user \"exit\\n\"} ", user, password, pts_filename);
  command = mysprintf( "spawn /bin/su - %s\nsleep 1 \nexpect -re \"Password: $\" { sleep 1; exp_send \"%s\\r\\n\"} \nsleep 1 \nexpect -re \" $\" { sleep 1; exp_send \"/tmp/get_pts %s\\r\\n\"} \nsleep 1 \nexpect \" $\" { sleep 1; exp_send \"exit\\r\"; send_user \"exit\\n\"} ", user, password, pts_filename);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  dataPtr->euid = dataPtr->ruid = uid;
  dataPtr->egid = dataPtr->rgid = 42;

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );


  // Get the pts used
	command = mysprintf("cat %s|awk -F/ '{print $4}' > %s", pts_filename, pts_filename2);
        printf("Running Command: %s\n", command);
	system(command);
	free(command);
  if( ( fPtr = fopen( pts_filename2, "r" ) ) == NULL ) {
    printf( "Error opening file [%s] for reading: errno = [%i]\n", pts_filename2, errno );
    rc = errno;
    goto EXIT;
  }

  if( fscanf( fPtr, "%d", &pts ) == 0 ) {
    printf( "No conversions assigned from file [%s]\n", pts_filename2 );
    rc = errno;
    goto EXIT;
  }
  fclose( fPtr );
  unlink(pts_filename);
  free(pts_filename);
  unlink(pts_filename2);
  free(pts_filename2);


  // Check for audit record(s)

  // uid/gid's are DONT CARES for the libpam tests, luid not yet set
  dataPtr->loginuid = dataPtr->euid = dataPtr->suid = dataPtr->ruid = dataPtr->fsuid = NO_ID_CHECK;
  dataPtr->loginuid = dataPtr->egid = dataPtr->sgid = dataPtr->rgid = dataPtr->fsgid = NO_ID_CHECK;

  strncpy(dataPtr->msg_evname, "AUTH_success", sizeof(dataPtr->msg_evname));
  dataPtr->comm = mysprintf("PAM authentication: user=%s (hostname=?, addr=?, terminal=pts/%d)", user, pts);
  verifyPAMProgram( dataPtr );

  strncpy(dataPtr->msg_evname, "AUTH_success", sizeof(dataPtr->msg_evname));
  dataPtr->comm = mysprintf("PAM accounting: user=%s (hostname=?, addr=?, terminal=pts/%d)", user, pts);
  verifyPAMProgram( dataPtr );


  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->comm );

  // End Test 1



  // Test 2: FAILURE su
  // When a user unsuccessfully uses the su program.
  // The record is generated with the following commands:
  // su (and a bad password)
  //
  // In addition to the standard audit information, the following string will be logged:
  // PAM authentication: user=USERNAME (hostname=?, addr=?, terminal=pts/PTS)
  //
  //
 //TEST_2:
  printf("TEST %d\n", test++);
  // Setup

  // Create expect script file to execute su session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "spawn /bin/su - %s\nsleep 1 \nexpect -re \"Password: $\" { exp_send \"%s\\r\\n\"} \nsleep 1 \nexpect \" $\" { exp_send \"exit\\r\"; send_user \"exit\\n\"} ", user, badpassword, pts_filename);

  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  dataPtr->euid = dataPtr->ruid = uid;
  dataPtr->egid = dataPtr->rgid = 42;

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );

  // Check for audit record(s)

  // uid/gid's are DONT CARES for the libpam tests, luid not yet set
  dataPtr->loginuid = dataPtr->euid = dataPtr->suid = dataPtr->ruid = dataPtr->fsuid = NO_ID_CHECK;
  dataPtr->loginuid = dataPtr->egid = dataPtr->sgid = dataPtr->rgid = dataPtr->fsgid = NO_ID_CHECK;

  strncpy(dataPtr->msg_evname, "AUTH_failure", sizeof(dataPtr->msg_evname));
  dataPtr->comm = mysprintf("PAM authentication: user=%s (hostname=?, addr=?, terminal=pts/%d)", user, pts);
  verifyPAMProgram( dataPtr );
  // NOTE: We're using the same pts number as in the success case.
  //       We don't know of a good way to definitively determine the pts of a user whose session connection failed.
  //       Thus, we're assuming that since this test closely follows the success case and since this is an isolated
  //       system, we can guess with some confidence that the pts will be the same as the session we just successfully
  //       opened and closed.   

  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->comm );

  // End Test 2

 EXIT:
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
  }
  free( command );
  printf("Returning from test_su()\n");
  return rc;
}


