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
void debug_expected(const laus_data *);

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

void debug_expected(const laus_data *dataPtr)
{
    //Dump all the expected data
    printf5("\tbegin     : %s", ctime(&(dataPtr->begin_r_time)));
    printf5("\tend       : %s", ctime(&(dataPtr->end_r_time)));
    printf5("\tseqnr     : %i\n", dataPtr->msg_seqnr);
    printf5("\ttype      : %i\n", dataPtr->msg_type);
    printf5("\tarch      : %i\n", dataPtr->msg_arch);
    printf5("\tpid       : %i\n", dataPtr->msg_pid);
    printf5("\tsize      : %zi\n", dataPtr->msg_size);
    printf5("\taudit id  : %i\n", dataPtr->msg_audit_id);
    printf5("\tluid      : %i\n", dataPtr->msg_login_uid);
    printf5("\teuid      : %i\n", dataPtr->msg_euid);
    printf5("\truid      : %i\n", dataPtr->msg_ruid);
    printf5("\tsuid      : %i\n", dataPtr->msg_suid);
    printf5("\tfsuid     : %i\n", dataPtr->msg_fsuid);
    printf5("\tegid      : %i\n", dataPtr->msg_egid);
    printf5("\trgid      : %i\n", dataPtr->msg_rgid);
    printf5("\tsgid      : %i\n", dataPtr->msg_sgid);
    printf5("\tfsgid     : %i\n", dataPtr->msg_fsgid);

    //Dump syscall specific data
    if (dataPtr->msg_type == AUDIT_MSG_SYSCALL) {

	printf5("\tcode      : %i\n", dataPtr->laus_var_data.syscallData.code);
	printf5("\tresult    : %i\n",
		dataPtr->laus_var_data.syscallData.result);
	printf5("\terrno     : %i\n",
		dataPtr->laus_var_data.syscallData.resultErrno);

	//Dump syscall parameters
	printf5("\tparameters\n");
	var_parms(5, dataPtr->laus_var_data.syscallData.code,
		  dataPtr->laus_var_data.syscallData.data);

	// Dump user space audit data
    } else if (dataPtr->msg_type == AUDIT_MSG_TEXT) {
	printf5("\ttext: %s\n", dataPtr->laus_var_data.textData.data);
    }
}
