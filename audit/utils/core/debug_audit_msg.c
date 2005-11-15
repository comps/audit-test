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
**  FILE   : audit_verify.c
**
**  PURPOSE: This file defines a very critical utility function for the
**           Linux Auditing System test suite.  This function analyzes
**           the audit logs and determines the accuracy of the log.
**           It is expected that the auditing system accurately records
**           successful and erroneous system calls, and it is this
**           function that will check such logs.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**    07/03 furthered by Kylene J. Smith <kylene@us.ibm.com>
**    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>
**
**********************************************************************/

#include "includes.h"
#include <linux/audit.h>
#include <time.h>

#define BUFSIZE 1024

//syscall specific functions
static void var_parms(int, int, const char *);

//generic
void debug_expected(const struct audit_data *);

/**********************
 ** HELPER FUNCTIONS **
 *********************/

/** OUTPUT TYPE **/

static
void var_parms(int level, int call, const char *src)
{
    char *buffer = NULL;
    char *vector_buffer = NULL;
    int buffer_index = 0;
    int arg_size = 0;
    ARG_TYPE arg_type = AUDIT_ARG_STRING;
    long long tmpint = 0;
    ARG_TYPE type = AUDIT_ARG_NULL;
    int size = 0;
    int rc;

    if (src != NULL) {
	buffer = (char *)malloc(BUFSIZE);
	memset(buffer, '\0', BUFSIZE);

	while ((rc = arg_get(&type, &size, buffer, &src)) != -1) {
	    switch (type) {
		case AUDIT_ARG_VECTOR:
		    fprintf(stderr, "\t\ttype = %i, size = %i\n", type, size);
		    vector_buffer = malloc(size);
		    while (buffer_index < size) {
			memcpy((char *)&arg_type, buffer + buffer_index,
			       sizeof(type));
			buffer_index += sizeof(type);
			memcpy((char *)&arg_size, buffer + buffer_index,
			       sizeof(size));
			buffer_index += sizeof(size);
			memset(vector_buffer, '\0', size);
			memcpy(vector_buffer, buffer + buffer_index, arg_size);
			fprintf(stderr,
				     "\t\t\ttype = %i, size = %i, data = %s\n",
				     arg_type, arg_size, vector_buffer);
			buffer_index += arg_size;
		    }
		    free(vector_buffer);
		    break;
		case AUDIT_ARG_NULL:
		    fprintf(stderr, "\t\ttype = %i, size = %i, data = NULL\n", 
			    type, size);
		    break;
		case AUDIT_ARG_ERROR:
		case AUDIT_ARG_IMMEDIATE:
		    memcpy(&tmpint, buffer, size);
		    tmpint = (long)tmpint;
		    fprintf(stderr, "\t\ttype = %i, size = %i, data = %Ld\n", 
			    type, size, tmpint);
		    break;
		case AUDIT_ARG_POINTER:
		    fprintf(stderr, "\t\ttype = %i, size = %i, ", type, size);
		    int idx;	//array index, set to start in each case
		    int bnd;	//array bounds, total num elements set in each case
		    void *grp_array = NULL;

		    switch (call) {
#ifndef NOXATTR
			case __NR_setxattr:
			case __NR_lsetxattr:
			case __NR_fsetxattr:
#endif
			case __NR_sethostname:
			case __NR_setdomainname:
			    bnd = size / sizeof(char);
			    grp_array = (void *)malloc(size);
			    memcpy(grp_array, buffer, size);
			    fprintf(stderr, "data=%.*s", bnd, 
				    ((char *)grp_array));
			    break;
#if !defined(__powerpc64__) && !defined(__x86_64__)
			case __NR_setgroups:
			    bnd = size / sizeof(u_int16_t);
			    idx = 0;
			    grp_array = (void *)malloc(size);
			    memcpy(grp_array, buffer, size);
			    for (idx = 0; idx < bnd; idx++)
				fprintf(stderr, "\n\t\t\tindex=%i, value=%i", 
					idx, ((u_int16_t *) grp_array)[idx]);
			    break;
#if !defined(__powerpc__) && !defined(__s390x__) && !defined(__ia64__)
			case __NR_setgroups32:

			    bnd = size / sizeof(gid_t);
			    idx = 0;
			    grp_array = (void *)malloc(size);
			    memcpy(grp_array, buffer, size);
			    for (idx = 0; idx < bnd; idx++)
				fprintf(stderr, "\n\t\t\tindex=%i, value=%i", 
					idx, ((gid_t *) grp_array)[idx]);
			    break;
#endif
#else
			case __NR_setgroups:
			    bnd = size / sizeof(gid_t);
			    idx = 0;
			    grp_array = (void *)malloc(size);
			    memcpy(grp_array, buffer, size);
			    for (idx = 0; idx < bnd; idx++)
				fprintf(stderr, "\n\t\t\tindex=%i, value=%i", 
					idx, ((gid_t *) grp_array)[idx]);
			    break;
#endif
			default:
			    memcpy(&tmpint, buffer, sizeof(tmpint));
			    tmpint = (long)tmpint;
			    fprintf(stderr, "data = %Lu", tmpint);
		    }
		    fprintf(stderr, "\n");
		    if (grp_array)
			free(grp_array);
		    break;
		default:
		    fprintf(stderr, "\t\ttype = %i, size = %i, data = %s\n", 
			    type, size, buffer);
		    break;
	    }
	    memset(buffer, '\0', BUFSIZE);
	}
	free(buffer);
    }
}

void debug_expected(const struct audit_data *context)
{
    fprintf(stderr, "\ttype      : %u\n", context->type);
    fprintf(stderr, "\tserial    : %u\n", context->serial);
    fprintf(stderr, "\ttimestamp : %s\n", ctime(&(context->timestamp)));
    fprintf(stderr, "\tbegin     : %s\n", ctime(&(context->begin_time)));
    fprintf(stderr, "\tend       : %s\n", ctime(&(context->end_time)));
    fprintf(stderr, "\tpid       : %i\n", context->pid);
    fprintf(stderr, "\tloginuid	 : %u\n", context->loginuid);
    fprintf(stderr, "\tuid       : %u\n", context->uid);
    fprintf(stderr, "\teuid      : %u\n", context->euid);
    fprintf(stderr, "\tsuid      : %u\n", context->suid);
    fprintf(stderr, "\tfsuid     : %u\n", context->fsuid);
    fprintf(stderr, "\tgid       : %u\n", context->gid);
    fprintf(stderr, "\tegid      : %u\n", context->egid);
    fprintf(stderr, "\tsgid      : %u\n", context->sgid);
    fprintf(stderr, "\tfsgid     : %u\n", context->fsgid);
    fprintf(stderr, "\tsuccess	 : %u\n", context->success);

    if (context->type == AUDIT_MSG_SYSCALL) {

	fprintf(stderr, "\tsyscall   : %s\n", context->u.syscall.sysname);
	fprintf(stderr, "\tsysnum    : %i\n", context->u.syscall.sysnum);
	fprintf(stderr, "\tarch      : %x\n", context->u.syscall.arch);
	fprintf(stderr, "\tpers      : %lx\n", context->u.syscall.pers);
	fprintf(stderr, "\texit      : %i\n", context->u.syscall.exit);

	fprintf(stderr, "\tparameters\n");
	var_parms(5, context->u.syscall.exit, context->u.syscall.args);

    } else if (context->type == AUDIT_MSG_TEXT) {
	fprintf(stderr, "\ttext: %s\n", context->u.user.buf);
    }
}
