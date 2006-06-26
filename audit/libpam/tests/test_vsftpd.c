
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
 **  FILE       : test_vsftpd.c
 **
 **  PURPOSE    : To test the vsftpd PAM program.
 **
 **  DESCRIPTION: The test_vsftpd() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs PAM program audit information for the
 **  "vsftpd" program.
 **  
 **  This function performs a series of tests on the vsftpd PAM
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

int test_vsftpd(struct audit_data* dataPtr) {

  int rc = 0;
  int test = 1;

  char* user;
  char* home;
  int uid;
  char* command;
  char* filename;
  int fd;

  // Produced with crypt( "eal", "42" ): 42VmxaOByKwlA
  char* password = "eal";
  char* encryptedpassword = "42VmxaOByKwlA";
  char* badpassword = "anything_but_eal";

  if ( ( rc = system("rpm -q vsftpd") ) != 0 ) {
    printf("vsftpd is not installed, assuming WS\n");
    fail_testcases++;
    goto WS;
  }

  dataPtr->euid = 0;
  dataPtr->egid = 0;
  if (( rc = createTempUserName( &user, &uid, &home ) == -1 )) {
    printf("Out of temp user names\n");
    goto EXIT;
  }
  // Create user
  command = mysprintf( "/usr/sbin/useradd -n -u %d -d %s -m -p %s %s", uid, home, encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // Modify configuration files
  backupFile("/etc/vsftpd/vsftpd.conf");
  // Warning: Possible /tmp attack vulnerability; we assume the test machine is in a controlled and secure test environment
  if ( ( rc = system( "grep -v \"local_enable\" /etc/vsftpd/vsftpd.conf > /tmp/vsftpd.conf ; mv -f /tmp/vsftpd.conf /etc/vsftpd/vsftpd.conf ; echo \"local_enable=YES\" >> /etc/vsftpd/vsftpd.conf" ) ) == -1 ) {
    printf( "Error modifying /etc/vsftpd/vsftpd.conf\n" );
    goto EXIT;
  }
  if ( ( rc = system( "/etc/init.d/vsftpd restart") ) != 0) {
    printf( "Error restarting vsftpd\n");
    goto EXIT;
  }


  // Test 1: SUCCESS ftp 
  // When a user successfully uses the vsftpd program.
  // The record is generated with the following commands:
  // ftp 
  //
  // In addition to the standard audit information, the following string will be logged:
  // PAM authentication: user=%s (hostname=127.0.0.1, addr=127.0.0.1, terminal=?)
  // PAM accounting: user=%s (hostname=127.0.0.1, addr=127.0.0.1, terminal=?)
  //
  //
 //TEST_1:
  printf("TEST %d\n", test++);
  // Setup
  // Create expect script file to execute ftp session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "spawn /usr/bin/ftp localhost\n"
"sleep 1 \n"
"expect \": $\" { exp_send \"%s\\r\\n\"} \n"
"expect \"Password:\" { exp_send \"%s\\r\\n\"} \n"
"expect \"ftp> $\" { exp_send \"quit\\r\"; send_user \"quit\\n\"} ", user, password);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->euid = dataPtr->suid = dataPtr->uid = dataPtr->fsuid = 0;
  dataPtr->egid = dataPtr->sgid = dataPtr->gid = dataPtr->fsgid = 0;
  dataPtr->pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );

  // Check for audit record

  // uid/gid's are DONT CARES for the libpam tests, luid not yet set
  dataPtr->loginuid = dataPtr->euid = dataPtr->suid = dataPtr->uid = dataPtr->fsuid = NO_ID_CHECK;
  dataPtr->loginuid = dataPtr->egid = dataPtr->sgid = dataPtr->gid = dataPtr->fsgid = NO_ID_CHECK;

  //strncpy(dataPtr->msg_evname, "AUTH_success", sizeof(dataPtr->msg_evname));
  dataPtr->type = AUDIT_MSG_USER;
  dataPtr->comm = mysprintf("PAM: authentication acct=%s : exe=./usr/sbin/vsftpd.*hostname=localhost.localdomain, addr=127.0.0.1, terminal=. res=success.*", user);
  verifyPAMProgram( dataPtr );

  //strncpy(dataPtr->msg_evname, "AUTH_success", sizeof(dataPtr->msg_evname));
  dataPtr->type = AUDIT_MSG_USER;
  dataPtr->comm = mysprintf("PAM: accounting acct=%s : exe=./usr/sbin/vsftpd.*hostname=localhost.localdomain, addr=127.0.0.1, terminal=. res=success.*", user);
  verifyPAMProgram( dataPtr );

  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->comm );

  // End Test 1


  // Test 1: FAILURE ftp
  // When a user successfully uses the vsftpd program.
  // The record is generated with the following commands:
  // ftp (with a bad password)
  //
  // In addition to the standard audit information, the following string will be logged:
  // PAM authentication: user=%s (hostname=127.0.0.1, addr=127.0.0.1, terminal=?)
  //
  //
 //TEST_2:
  printf("TEST %d\n", test++);
  // Setup
  // Create expect script file to execute ftp session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "spawn /usr/bin/ftp localhost\n"
"sleep 1 \n"
"expect \": $\" { exp_send \"%s\\r\\n\"} \n"
"expect \"Password:\" { exp_send \"%s\\r\\n\"} \n"
"expect \"ftp> $\" { exp_send \"quit\\r\"; send_user \"quit\\n\"} ", user, badpassword);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->euid = dataPtr->suid = dataPtr->uid = dataPtr->fsuid = 0;
  dataPtr->egid = dataPtr->sgid = dataPtr->gid = dataPtr->fsgid = 0;
  dataPtr->pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );

  // Check for audit record

  // uid/gid's are DONT CARES for the libpam tests, luid not yet set
  dataPtr->loginuid = dataPtr->euid = dataPtr->suid = dataPtr->uid = dataPtr->fsuid = NO_ID_CHECK;
  dataPtr->loginuid = dataPtr->egid = dataPtr->sgid = dataPtr->gid = dataPtr->fsgid = NO_ID_CHECK;

  //strncpy(dataPtr->msg_evname, "AUTH_failure", sizeof(dataPtr->msg_evname));
  dataPtr->type = AUDIT_MSG_USER;
  dataPtr->comm = mysprintf("PAM: authentication acct=%s : exe=./usr/sbin/vsftpd.*hostname=localhost.localdomain, addr=127.0.0.1, terminal=. res=failed.*", user);
  verifyPAMProgram( dataPtr );

  // Cleanup
  unlink( filename );
  free( filename );
  free( dataPtr->comm );

  // End Test 2

 EXIT:
  restoreFile("/etc/vsftpd/vsftpd.conf");
  if ( ( rc = system( "/etc/init.d/vsftpd stop") ) != 0) {
    printf( "Error restarting vsftpd\n");
    goto EXIT;
  }

  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf( "Error deleting user [%s]\n", user );
  }
  free( command );
 WS:
  printf("Returning from test_vsftpd()\n");
  return !!fail_testcases;
}


