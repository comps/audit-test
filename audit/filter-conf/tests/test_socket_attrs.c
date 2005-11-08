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
**  FILE       : test_socket_attrs.c
**
**  PURPOSE    : Used to test the audit filter configuration.
**
**  DESCRIPTION: This file contains various routines used to test
**  the filter configuration support of socket attributes.  The
**  functions build into the laus_test framework to provide a basis
**  to verify that the Linux Audit System accurately logs the system
**  call based upon the filter configuration.
**
**  Each function will documents its execution.
**
**  HISTORY    :
**    08/03 Originated by Tom Lendacky (toml@us.ibm.com)
**    05/04 Adapted for EAL4 by Dan Jones (danjones@us.ibm.com)
**
**********************************************************************/

#include "includes.h"

#include "filterconf.h"
#include "filter_call_utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/net.h>


/**********************************************************************
**  Function: test_socket
**    Tests the sock-family, sock-type socket attribute support.
**
**   1) Performs pre-processing, including setting the filter
**      configuration
**   2) Executes the socket and bind system calls with pre-determined
**      parameters.
**   3) Performs post-processing, including saving the results for
**      verification.
**
**********************************************************************/
int test_socket(laus_data *ld, uid_t uid, gid_t gid) {

	int rc = 0;

	int sd = 0;
	int portNum = -1;
	char *sd_desc = NULL;
	socklen_t addrlen = 0;
	struct sockaddr_in my_addr;


	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		printf1("ERROR: socket failed - errno=%d %s\n", errno, strerror(errno));
		rc = -1;
		goto EXIT;
	}

	// Do pre-system call work
	printf5("Calling pre_filter_call\n");
	if ((rc = pre_filter_call(ld)) != 0) {
		printf1("ERROR: pre_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

	my_addr.sin_port = htons( -1 );
	my_addr.sin_family = AF_INET;
	portNum = 55555; // Initial port number to try
	my_addr.sin_port = htons( portNum );
	my_addr.sin_addr.s_addr = INADDR_ANY;
	addrlen = sizeof( struct sockaddr_in );
   
	// Now, find a port that works
	while( bind( sd, (struct sockaddr*)&my_addr, addrlen ) == -1 && 
	       portNum < 60000 ) {
	  printf1( "Attempt to bind to port %d failed; trying one higher\n", portNum );
	  portNum++;
	  my_addr.sin_port = htons( portNum );
	}
	if( portNum == 60000 ) {
	  printf1( "ERROR: Failed to bind to any port between 55555 and 60000\n" );
	  goto EXIT;
	} 
	close( sd );
	sd_desc = mysprintf("[sock:af=%d,type=%d]", PF_INET, SOCK_STREAM);

	// Set up audit argument buffer
	// Since bind() test is called a little differently than the rest of the tests,
	if(( rc = auditArg3(ld, 
			    AUDIT_ARG_PATH, strlen(sd_desc), sd_desc,
			    AUDIT_ARG_POINTER, sizeof(struct sockaddr), &my_addr,
			    AUDIT_ARG_IMMEDIATE, sizeof(socklen_t), &addrlen)) != 0) {
	  printf1("ERROR: auditArg3 failed - rc=%d\n", rc);
	  goto EXIT;
	} 

	// Set the data
	ld->msg_type = AUDIT_MSG_SYSCALL;
	ld->laus_var_data.syscallData.code = AUDIT_bind;
	ld->laus_var_data.syscallData.result = rc;
	ld->laus_var_data.syscallData.resultErrno = errno;

	// Do post-system call work
	printf5("Calling post_filter_call\n");
	if ((rc = post_filter_call(ld)) != 0) {
		printf1("ERROR: post_filter_call failed - rc=%d\n", rc);
		goto EXIT;
	}

EXIT:
	// Do cleanup work here
	if (sd_desc)
		free(sd_desc);

	printf5("Returning from test - rc=%d\n", rc);

	return rc;
}
