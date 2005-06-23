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
**  FILE   : audit_config.c
**
**  PURPOSE: This file defines a function that configures the
**           auditing system according to the setup that is to be
**           tested.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
**
**********************************************************************/

#include "utils.h"

/*
** Setup the filter domain
**
** The purpose of this function is to setup the audit
** config file such that the audit subsystem will either
** log success, log failure, log both or log none.
**
** The function works by editing a template filter.conf
** file with the desired logging directive.
*/

#define LOG_ALL  "always"
#define LOG_NONE "never"
#define LOG_GOOD "success"
#define LOG_BAD  "failed"

int setFilterDomain(log_options logOption) {

  int rc = 0;

#ifdef __X86_64
  //will create syscall-success, syscall-addr-success, syscall-shmat-success or failed equivs. wanted on Opt
  char* opt_command = "sed \"s/#dummy#/";    
#else
#ifdef __IX86
  //will create syscall-success, syscall-addr-success
  char* x_command = "sed \"s/[shmat-]*#dummy#/-";
#else
  //will create syscall-success wanted on zip
  char* zip_command    = "sed \"s/[adrshmt-]*#dummy#/-";   // not needed?
#endif
#endif


//generic for never and always
  char* abs_command     = "sed \"s/syscall-.*#dummy#/";
  char* end_command="/\" ../utils/filter.conf >/etc/audit/filter.conf";
  char action[strlen(LOG_GOOD) + 1];
  char complete_command[1024];

#ifndef NOSLEEP
  sleep(2);
#endif

  if ( (rc = chdir( cwd )) == -1 ) {
      printf ( "Error changing to working directory [%s]: errno:%i\n",
	       cwd, errno);
      goto EXIT;
  }

  printf4("setFilterDomain: success=%i failure=%i\n", 
	 logOption.logSuccess, logOption.logFailure);

  if ( logOption.logSuccess && logOption.logFailure ) {
      strcpy(complete_command, abs_command);
      strcpy(action, LOG_ALL);
  }

  else if ( logOption.logSuccess && !logOption.logFailure ) {
#ifdef __X86_64
      strcpy(complete_command, opt_command);
#else
#ifdef __IX86
      strcpy(complete_command, x_command);
#else
      strcpy(complete_command, zip_command);
#endif
#endif
      strcpy(action, LOG_GOOD);
  }
  else if ( !logOption.logSuccess && logOption.logFailure ) {
#ifdef __X86_64
      strcpy(complete_command, opt_command);
#else
#ifdef __IX86
      strcpy(complete_command, x_command);
#else
      strcpy(complete_command, zip_command);
#endif
#endif
      strcpy(action, LOG_BAD);
  }
  else {
      strcpy(complete_command, abs_command);
      strcpy(action, LOG_NONE);
  }

  //  strcpy(complete_command, begin_command);
  strcat(complete_command, action);
  strcat(complete_command, end_command);
  
  printf5("Command about to execute is:\n\t%s\n",complete_command);

  if ( (rc = system(complete_command) ) == -1 ) {
      printf1("Unable to change configuration appropriately\n");
      goto EXIT;
  }

  printf5( "Reloading the audit\n" );
  if( ( rc = reloadAudit() ) == -1 ) {
    printf1( "Error reloading the audit : rc=%i\n", rc );
    goto EXIT;
  }

#ifndef NOSLEEP
  sleep(2);
#endif

EXIT:
  return rc;
}
