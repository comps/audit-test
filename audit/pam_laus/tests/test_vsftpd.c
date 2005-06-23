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

#include "../include/pam_laus.h"
#include "tempname.h"
#include <time.h>
#include <pwd.h>
#include <sys/utsname.h>

int test_vsftpd(laus_data* dataPtr) {

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
  //char* badpassword = "anything_but_eal";
  char* executable;

  executable = "/usr/sbin/vsftpd";

  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  if (( rc = createTempUserName( &user, &uid, &home ) == -1 )) {
    printf1("Out of temp user names\n");
    goto EXIT;
  }
  // Create user
  command = mysprintf( "/usr/sbin/useradd -u %d -d %s -m -p %s %s", uid, home, encryptedpassword, user );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error creating user [%s]\n", user );
    goto EXIT;
  }
  free( command );
  // Modify configuration files
  backupFile("/etc/vsftpd.conf");
  // machine is in a controlled and secure test environment
  if ( ( rc = system( "echo \"local_enable=YES\" > /etc/vsftpd.conf; echo \"pam_service_name=vsftpd\" >> /etc/vsftpd.conf" ) ) == -1 ) {
	  printf1( "Error modifying /etc/vsftpd.conf\n" );
	  goto EXIT;
  }
/* MH: This isn't the test's job
  backupFile("/etc/pam.d/vsftpd");
  if ( ( rc = system( "cat /etc/pam.d/vsftpd | grep -v laus_laus.so > /etc/pam.d/vsftpd; echo \"auth required pam_laus.so detach\" >> /etc/pam.d/vsftpd") ) == -1 ) {
    printf1( "Error modifying /etc/pam.d/vsftpd\n" );
    goto EXIT;
  }
  backupFile("/etc/xinetd.conf");
  if ( ( rc = system( "echo \"ftp stream tcp nowait root /usr/sbin/tcpd vsftpd\" > /etc/xinetd.conf") ) == -1 ) {
    printf1( "Error modifying /etc/xinetd.conf\n" );
    goto EXIT;
  }
  if ( rc = system("/etc/init.d/xinetd restart") == -1 ) {
    printf1("Error restarting xinetd\n");
    goto EXIT;
  }
*/


  // Test 1: SUCCESS ftp 
  // When a user successfully uses the vsftpd program.
  // The record is generated with the following commands:
  // vsftpd 
  //
  //
 //TEST_1:
  printf5("TEST %d\n", test++);
  // Setup
  // Create expect script file to execute ftp session
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "spawn /usr/bin/ftp localhost\nsleep 1 \nexpect \": $\" { exp_send \"%s\\r\\n\"} \nexpect \"Password:\" { exp_send \"%s\\r\\n\"} \nexpect \"ftp> $\" { exp_send \"quit\\r\"; send_user \"quit\\n\"} ", user, password);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->msg_pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );

  // Set expected data for audit record
  // uid/gid's are DONT CARES 
  dataPtr->msg_login_uid = dataPtr->msg_euid = dataPtr->msg_suid = dataPtr->msg_ruid = dataPtr->msg_fsuid = NO_ID_CHECK;
  dataPtr->msg_login_uid = dataPtr->msg_egid = dataPtr->msg_sgid = dataPtr->msg_rgid = dataPtr->msg_fsgid = NO_ID_CHECK;
  dataPtr->laus_var_data.loginData.uid = uid;
  strcpy(dataPtr->laus_var_data.loginData.hostname, "127.0.0.1"); // vsftpd uses the ip address here, instead of hostname
  strcpy(dataPtr->laus_var_data.loginData.address, "127.0.0.1");
  strcpy(dataPtr->laus_var_data.loginData.executable, executable);

  // Check for audit record
  verifyPAMProgram( dataPtr );

  // Cleanup
  unlink( filename );
  free( filename );

  // End Test 1


 EXIT:
  restoreFile("/etc/vsftpd.conf");
/*
  restoreFile("/etc/pam.d/vsftpd");
  restoreFile("/etc/xinetd.conf");
  if ( rc = system("/etc/init.d/xinetd restart") == -1 ) {
    printf1("Error restarting xinetd\n");
  }
*/
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error deleting user [%s]\n", user );
  }
  free( command );
  printf5("Returning from test_vsftpd()\n");
  return rc;
}


