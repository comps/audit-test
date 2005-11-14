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
    **  FILE       : test_bind.c
    **
    **  PURPOSE    : To test the bind library call auditing.
    **
    **  DESCRIPTION: The test_bind() function builds into the
    **  laus_test framework to verify that the Linux Audit System
    **  accurately logs both successful and erroneous execution of the
    **  "bind" system call.
    **
    **  In the successful case, this function:
    **   1) Creates a new socket
    **   2) Finds a valid port for binding
    **   3) Sets the euid to the test user
    **   4) Makes the bind syscall
    **   5) Sets the euid to the superuser
    **   6) Verifies that bind executed successfully
    **   7) Closes the socket.
    **
    **  The successful case creates an ordinary INET TCP socket and binds
    **  to it.  It sets up the data structures and parameters according
    **  to the instructions given in the man pages for socket and bind.
    **  
    **  In the erroneous case, this function:
    **   1) Creates a new socket
    **   2) Sets the euid to the test user
    **   3) Calls bind() with the sockfd being -1 
    **   4) Sets the euid to the superuser
    **   5) Verifies that bind executed erroneously
    **   6) Closes the socket.
    **      
    **  The erroneous case passes a -1 for socket file descriptor.  Since
    **  this is a bad file descriptor, bind will return an EBADF error code.
    **
    **  HISTORY    :
    **    06/03 Originated by Michael A. Halcrow <mike@halcrow.us>
    **    07/03 Furthered by Dustin Kirkland <k1rkland@us.ibm.com>
    **    05/04 Updates to supress warnings by Kimberly D. Simn <kdsimon@us.ibm.com>
    **
    **********************************************************************/

#include "includes.h"
#include "syscalls.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/net.h>

int test_bind(struct audit_data *context)
{


    int rc = 0;
#if defined(__S390X) || defined(__PPC64) || defined(__X86_64) || defined(__IA64)
    __s64 exp_errno = EBADF;
#else
    int exp_errno = EBADF;
#endif
    int sockfd;
    struct sockaddr_in my_addr;
    //struct sockaddr_in* null_ptr = NULL;   // not needed?
    socklen_t addrlen = 0;
    int portNum = -1;
    char SocketPath[19];

     /**
      * Do as much setup work as possible right here
      */
    if (context->success) {
	my_addr.sin_port = htons(-1);
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
	    printf1("Error creating socket\n");
	    goto EXIT;
	}
	// fill in my_addr
	my_addr.sin_family = AF_INET;
	portNum = 54348;	// Initial port number to try
	my_addr.sin_port = htons(portNum);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	// set addrlen
	addrlen = sizeof(struct sockaddr_in);

	// Now, find a port that works
	while (bind(sockfd, (struct sockaddr *)&my_addr, addrlen) == -1 &&
	       portNum < 60000) {
	    printf3("Attempt to bind to port %d failed; trying one higher\n",
		    portNum);
	    portNum++;
	    my_addr.sin_port = htons(portNum);
	}
	if (portNum == 60000) {	// Okay, so we *may* have bound to port
	    // 60000, but the likelihood of that
	    // happening is very small if we were
	    // unable to bind to the other 5,651
	    // ports, so we ignore that case.
	    printf1
		("Failed to bind to any port between 54348 and 60000; something must be wrong\n");
	    goto EXIT_CLEANUP;
	} else {
	    // We have successfully ``binded'' to the socket; 
	    // OK use current configuration for the audit test
	    close(sockfd);
	    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		printf1("Error creating socket\n");
		goto EXIT;
	    }
	    sprintf(SocketPath, "[sock:af=%d,type=%d]", PF_INET, SOCK_STREAM);
	}
    } else {
	// set up non-success case
	sockfd = -1;
    }

    // Set up audit argument buffer
    // Since bind() test is called a little differently than the rest
    // of the tests,
    if (context->success)
	rc = auditArg3(context,
		       AUDIT_ARG_PATH, strlen(SocketPath), (void *)SocketPath,
		       AUDIT_ARG_POINTER, sizeof(struct sockaddr), &my_addr,
		       AUDIT_ARG_IMMEDIATE, sizeof(socklen_t), &addrlen);
    else
	rc = auditArg3(context,
		       AUDIT_ARG_ERROR, sizeof(__u64), (void *)&exp_errno,
		       AUDIT_ARG_NULL, 0, &my_addr,
		       AUDIT_ARG_IMMEDIATE, sizeof(socklen_t), &addrlen);
    if (rc != 0) {
	printf1("Error setting up audit argument buffer\n");
	goto EXIT;
    }

    // Do pre-system call work
    if ((rc = preSysCall(context)) != 0) {
	printf1("ERROR: pre-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }
    // Execute system call
    // syscall() does not cooperate with the bind() system call, so call bind() directly
    context->u.syscall.exit =
	bind(sockfd, (struct sockaddr *)&my_addr, addrlen);


    // Do post-system call work
    if ((rc = postSysCall(context, errno, -1, exp_errno)) != 0) {
	printf1("ERROR: post-syscall setup failed (%d)\n", rc);
	goto EXIT_CLEANUP;
    }

EXIT_CLEANUP:
     /**
      * Do cleanup work here
      */
    if (context->success) {
	close(sockfd);
    }

EXIT:
    printf5("Returning from test\n");
    return rc;
}
