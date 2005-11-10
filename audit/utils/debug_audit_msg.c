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
		    printf_level(level, "\t\ttype = %i, size = %i\n", type,
				 size);
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
			printf_level(level,
				     "\t\t\ttype = %i, size = %i, data = %s\n",
				     arg_type, arg_size, vector_buffer);
			buffer_index += arg_size;
		    }
		    free(vector_buffer);
		    break;
		case AUDIT_ARG_NULL:
		    printf_level(level,
				 "\t\ttype = %i, size = %i, data = NULL\n",
				 type, size);
		    break;
		case AUDIT_ARG_ERROR:
		case AUDIT_ARG_IMMEDIATE:
		    memcpy(&tmpint, buffer, size);
		    tmpint = (long)tmpint;
		    printf_level(level,
				 "\t\ttype = %i, size = %i, data = %Ld\n", type,
				 size, tmpint);
		    break;
		case AUDIT_ARG_POINTER:
		    printf_level(level, "\t\ttype = %i, size = %i, ", type,
				 size);
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
			    printf_level(level, "data=%.*s", bnd,
					 ((char *)grp_array));
			    break;
#if !defined(__PPC64) && !defined(__X86_64)
			case __NR_setgroups:
			    bnd = size / sizeof(u_int16_t);
			    idx = 0;
			    grp_array = (void *)malloc(size);
			    memcpy(grp_array, buffer, size);
			    for (idx = 0; idx < bnd; idx++)
				printf_level_prime(level,
						   "\n\t\t\tindex=%i, value=%i",
						   idx,
						   ((u_int16_t *)
						    grp_array)[idx]);
			    break;
#if !defined(__PPC) && !defined(__S390X) && !defined(__IA64)
			case __NR_setgroups32:

			    bnd = size / sizeof(gid_t);
			    idx = 0;
			    grp_array = (void *)malloc(size);
			    memcpy(grp_array, buffer, size);
			    for (idx = 0; idx < bnd; idx++)
				printf_level_prime(level,
						   "\n\t\t\tindex=%i, value=%i",
						   idx,
						   ((gid_t *) grp_array)[idx]);
			    break;
#endif
#else
			case __NR_setgroups:
			    bnd = size / sizeof(gid_t);
			    idx = 0;
			    grp_array = (void *)malloc(size);
			    memcpy(grp_array, buffer, size);
			    for (idx = 0; idx < bnd; idx++)
				printf_level_prime(level,
						   "\n\t\t\tindex=%i, value=%i",
						   idx,
						   ((gid_t *) grp_array)[idx]);
			    break;
#endif
			default:
			    memcpy(&tmpint, buffer, sizeof(tmpint));
			    tmpint = (long)tmpint;
			    printf_level_prime(level, "data = %Lu", tmpint);
		    }
		    printf_level_prime(level, "\n");
		    if (grp_array)
			free(grp_array);
		    break;
		default:
		    printf_level(level, "\t\ttype = %i, size = %i, data = %s\n",
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
    printf5("\ttype      : %u\n", context->type);
    printf5("\tserial    : %u\n", context->serial);
    printf5("\ttimestamp : %s\n", ctime(&(context->timestamp)));
    printf5("\tbegin     : %s\n", ctime(&(context->begin_time)));
    printf5("\tend       : %s\n", ctime(&(context->end_time)));
    printf5("\tpid       : %i\n", context->pid);
    printf5("\tloginuid	 : %u\n", context->loginuid);
    printf5("\tuid       : %u\n", context->uid);
    printf5("\teuid      : %u\n", context->euid);
    printf5("\tsuid      : %u\n", context->suid);
    printf5("\tfsuid     : %u\n", context->fsuid);
    printf5("\tgid       : %u\n", context->gid);
    printf5("\tegid      : %u\n", context->egid);
    printf5("\tsgid      : %u\n", context->sgid);
    printf5("\tfsgid     : %u\n", context->fsgid);
    printf5("\tsuccess	 : %u\n", context->success);

    if (context->type == AUDIT_MSG_SYSCALL) {

	printf5("\tsyscall   : %s\n", context->u.syscall.sysname);
	printf5("\tsysnum    : %i\n", context->u.syscall.sysnum);
	printf5("\tarch      : %x\n", context->u.syscall.arch);
	printf5("\tpers      : %lx\n", context->u.syscall.pers);
	printf5("\texit      : %i\n", context->u.syscall.exit);

	printf5("\tparameters\n");
	var_parms(5, context->u.syscall.exit, context->u.syscall.args);

    } else if (context->type == AUDIT_MSG_TEXT) {
	printf5("\ttext: %s\n", context->u.user.buf);
    }
}
