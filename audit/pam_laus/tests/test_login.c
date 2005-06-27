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
 **  FILE       : test_login.c
 **
 **  PURPOSE    : To test the login PAM program.
 **
 **  DESCRIPTION: The test_login() function builds into the
 **  laus_test framework to verify that the Linux Audit System
 **  accurately logs PAM program audit information for the
 **  "login" program.
 **  
 **  This function performs a series of tests on the login PAM
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

#include "pam_laus.h"
#include "tempname.h"
#include <time.h>
#include <pwd.h>

#include <utmp.h>
int userdel_utmp(char* user) {
// HACK!!!  
// This function provides a much need (though decisively ugly HACK.
// The /bin/login utility, when called by the expect script
// within this test, logs a user in, who then subsequently logs out.
// The log out, however, is not entirely complete.  Yes, all processes
// and the like are killed, however, for no apparent reason, an entry
// lingers in the /var/run/utmp file.  This entry causes the userdel
// of the temp user to fail.  Thus, this script will create multiple
// temp users upon adjacent runs (without reboots in between).
// 
// This function actually edits the /var/run/utmp file and zeros out
// the temp user's entry so that the userdel cleanup command can proceed.
  struct utmp entity;
  int fp;
  if ( ( fp = open( "/var/run/utmp", O_RDWR ) ) >=0 ) {
    while ( read ( fp, &entity, sizeof( entity ) ) > 0 ) {
      if ( strncmp( entity.ut_name, user, strlen( user ) ) == 0 ) {
        memset( (char *)&entity, 0, sizeof( entity ) );
        lseek ( fp, -(sizeof (entity)), SEEK_CUR );
        write ( fp, &entity, sizeof( entity ) );
      }
    }
    close(fp);
  }
  return 0;
}


int test_login(laus_data* dataPtr) {

  int rc = 0;
  int test = 1;

  char* user;
  char* home;
  int uid;
  char* command;
  char* filename;
  char* pts_filename;
  int pts;
  int fd;
  FILE* fPtr;

  // Produced with crypt( "eal", "42" ): 42VmxaOByKwlA
  char* password = "eal";
  char* encryptedpassword = "42VmxaOByKwlA";
  char* badpassword = "anything_but_eal";

#ifdef __PPC
  char* executable = "/lib/libnss_compat.so.2";
#else
  char* executable = "/bin/login";
#endif

  dataPtr->msg_euid = 0;
  dataPtr->msg_egid = 0;
  if ( rc = createTempUserName( &user, &uid, &home ) == -1 ) {
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
  // MH: This isn't the test's job.
//  backupFile("/etc/pam.d/login");
//  if ( ( rc = system( "cat /etc/pam.d/login | grep -v pam_laus > /etc/pam.d/login; echo \"auth required pam_laus.so detach\" >> /etc/pam.d/login") ) == -1 ) {
//    printf1( "Error modifying /etc/pam.d/login\n" );
//    goto EXIT;
//  }


  // Test 1: SUCCESS login 
  // When a user successfully uses the login program.
  // The record is generated with the following commands:
  // login 
  //
  //
 TEST_1:
  printf5("TEST %d\n", test++);
  // Setup
  // Create expect script file to execute login session
  createTempFileName(&pts_filename);
  filename = (char *) malloc(strlen(tempname));
  strcpy(filename, tempname);
  fd = mkstemp(filename);
  command = mysprintf( "spawn /bin/login \n\
sleep 1 \n\
expect -re \"login: \" { exp_send \"%s\\r\"} \n\
sleep 1\n\
expect -re \"Password: \" { exp_send \"%s\\r\"} \nsleep 1\n\
expect -re \" $\" { exp_send \"/usr/bin/tty | /usr/bin/awk -F/ '{print \\$4}' > %s\\r\"} \n\
sleep 1\n\
expect -re \" $\" { exp_send \"exit\\r\"; send_user \"exit\\n\"}\n",
			user, password, pts_filename);
  write(fd, command, strlen(command));
  fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO);
  close(fd);
  free( command );

  // Execution
  command = mysprintf( "/usr/bin/expect -f %s", filename );
  dataPtr->msg_pid = NO_FORK;
  runPAMProgram( dataPtr, command );
  free( command );

  // Get the pts used
  fPtr = fopen(pts_filename, "r");
  fscanf(fPtr, "%d", &pts);
  fclose(fPtr);
  unlink(pts_filename);
  free(pts_filename);

  // Set expected audit information
  // uid/gid's are DONT CARES
  dataPtr->msg_login_uid = dataPtr->msg_euid = dataPtr->msg_suid = dataPtr->msg_ruid = dataPtr->msg_fsuid = NO_ID_CHECK;
  dataPtr->msg_login_uid = dataPtr->msg_egid = dataPtr->msg_sgid = dataPtr->msg_rgid = dataPtr->msg_fsgid = NO_ID_CHECK;
  dataPtr->laus_var_data.loginData.uid = uid;
  command = mysprintf("/dev/pts/%d", pts);
  strcpy(dataPtr->laus_var_data.loginData.terminal, command);
  free(command);
  strcpy(dataPtr->laus_var_data.loginData.executable, executable);

  // Check for audit record
  verifyPAMProgram( dataPtr );

  // Cleanup
  unlink( filename );
  free( filename );

  // End Test 1

 EXIT:
  restoreFile("/etc/pam.d/login");
  userdel_utmp( user );			// See the HACK note in the function definition above
  command = mysprintf( "/usr/sbin/userdel -r %s", user );
  if( ( rc = system( command ) ) == -1 ) {
    printf1( "Error deleting user [%s]\n", user );
  }
  free( command );
  printf5("Returning from test_login()\n");
  return rc;
}


