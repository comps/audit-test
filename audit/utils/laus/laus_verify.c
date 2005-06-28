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
#include "audit_read.h"

#define BUFSIZE 1024

//syscall specific functions
void var_parms (int, int, const char*); 
int compare_var_parms(const char*, const char*);
void sycl_info(int, const struct aud_msg_syscall*, const laus_data*);

//generic
void msg_info (int, const struct aud_message_on_disk*, const laus_data*);
void debug_expected( const laus_data*);

/*
** Verify the audit log record
**
** BUGBUG:
** I expect this function to evolve into a fairly complex piece
** of code that will need to use the liblaussrv library to extract
** audit records.
**
** The function will need to determine if a log record should exist
** based on the value of successCase and the logOption (i.e.
** logSuccess/logFailure).
**
** If a log record is available, the data in the record must be
** consistent with the expected data in the laus_data structure.
**
** The pass/fail counters are maintained by this function since this
** is where the pass/fail check is made.
*/
int verifyLog(laus_data* dataPtr, log_options logOption) {

#if !defined(__IX86)
struct aud_msg_exit_on_disk {
        __laus_int64 code;
};
#else
#define aud_msg_exit_on_disk aud_msg_exit
#endif

    int rc        = 0;

    struct aud_log log;
    struct aud_message_on_disk * msg = NULL;
    struct laus_record_header_on_disk hdr;
    struct aud_msg_syscall * sycl = NULL;
    struct aud_msg_exit_on_disk * exit = NULL;
    struct aud_msg_netlink * netlink = NULL;
    struct aud_msg_login * login = NULL;
    char * usrdata = NULL;
    int record_found = FALSE;
    int len;
    int count = 0;
    time_t r_time;
	
    printf4("verifyLog\n");

    //debug_expected(dataPtr);

    //REAL AUDIT VERIFY SETUP
    initialize_aud_log(&log);



 AUDIT_BEGIN:
    if( (rc = open_audit_log(&log, "/var/log/audit") ) < 0 ) { //bad assumption
	printf1("Error: Unable to open audit log\n");
	goto OUT_NOTOPEN;
    }

    while ( ( msg = get_next_audit_record(&log, &hdr) ) != NULL ) {

	//Loop structure: negative cases continue to next record

	//common area: ********* don't do anything syscall specfic in this area

	//must be correct msg type or we don't care
	if (  msg->msg_type != dataPtr->msg_type ) {
	    continue;
	}

	//if syscall must be the correct number & type or we don't care
	if ( ( msg->msg_type == AUDIT_MSG_SYSCALL ) &&
	     ( ( ((struct aud_msg_syscall*)&msg->msg_data)->major ) !=
	       dataPtr->laus_var_data.syscallData.code ) ) {
	    continue;
	}

	//not to our time frame in the log yet if:
	r_time = hdr.r_time;
	if ( r_time < dataPtr->begin_r_time ) {
	    continue;
	}

	//record not found if:
	//if ( hdr.r_time > ( dataPtr->end_r_time ) ) {
	    //printf5("Passed expected time, no record found\n");
	    //record_found = FALSE;
	    //goto AUDIT_RETRY;
	//  printf9("Message passed time @ seqnr = %d\n", msg->msg_seqnr);
	//  continue;
	//}

	//message not a match if: (msg info)
	// msg_seqnr, msg_size, msg_audit_id expected value not available 
	if ( 
	    //( msg->msg_type != dataPtr->msg_type ) ||  //tested above
	    ( msg->msg_arch != dataPtr->msg_arch ) ||
	    ( ( dataPtr->msg_pid != NO_PID_CHECK ) &&
	      ( msg->msg_pid != dataPtr->msg_pid ) ) ||
	    ( ( dataPtr->msg_login_uid != NO_ID_CHECK ) &&
	      ( msg->msg_login_uid != dataPtr->msg_login_uid ) ) ||
	    ( ( dataPtr->msg_euid != NO_ID_CHECK ) &&
	      ( msg->msg_euid != dataPtr->msg_euid ) ) ||
	    ( ( dataPtr->msg_ruid != NO_ID_CHECK ) &&
	      ( msg->msg_ruid != dataPtr->msg_ruid ) )||
	    ( ( dataPtr->msg_suid != NO_ID_CHECK ) &&
	      ( msg->msg_suid != dataPtr->msg_suid ) ) ||
	    ( ( dataPtr->msg_fsuid != NO_ID_CHECK ) &&
	      ( msg->msg_fsuid != dataPtr->msg_fsuid ) ) ||
	    ( ( dataPtr->msg_egid != NO_ID_CHECK ) &&
	      ( msg->msg_egid != dataPtr->msg_egid ) ) ||
	    ( ( dataPtr->msg_rgid != NO_ID_CHECK ) &&
	      ( msg->msg_rgid != dataPtr->msg_rgid ) ) ||
	    ( ( dataPtr->msg_sgid != NO_ID_CHECK ) &&
	      ( msg->msg_sgid != dataPtr->msg_sgid ) ) ||
	    ( ( dataPtr->msg_fsgid != NO_ID_CHECK ) &&
	      ( msg->msg_fsgid != dataPtr->msg_fsgid ) )
	    ) {	    
	    printf9("Data pointer and message values didn't match:\n");
	    msg_info(9, msg, dataPtr);
	    continue;
	}
	// **********   common area end   **********

	switch ( dataPtr->msg_type ) {
	case AUDIT_MSG_SYSCALL:

	  if( dataPtr->laus_var_data.syscallData.personality == NO_CHECK_SYSCALL_DATA ) {
	    printf4( "Flag set indicating that no log verify be performed for this syscall." );
	    continue;
	  }

	    sycl = (struct aud_msg_syscall *)&msg->msg_data;

	    //message not a match if: (system call info)
	    if (
		( sycl->personality !=
		  dataPtr->laus_var_data.syscallData.personality ) ||
		//( sycl->major != dataPtr->laus_var_data.syscallData.code ) || //tested above
		( sycl->minor != dataPtr->laus_var_data.syscallData.minor ) ||
		( sycl->length != dataPtr->laus_var_data.syscallData.length )
		) {
		printf9("System call specific information didn't match:\n");
		printf9("\tBegin Range: %s", ctime(&(dataPtr->begin_r_time)));
		printf9("\tEnd   Range: %s", ctime(&(dataPtr->end_r_time)));
		printf9("Message:    %s", ctime(&(r_time)));
		printf9("Seqnr: %d\n", msg->msg_seqnr); 
		printf9("\tActual:\n");
		var_parms( 9, sycl->major, sycl->data );
		printf9("\tExpected:\n");
		var_parms( 9, dataPtr->laus_var_data.syscallData.code,
			   dataPtr->laus_var_data.syscallData.data);
		sycl_info(9, sycl, dataPtr);
		continue;
	    }
	    
	    //message not a match if: (result & errno)
	    if ( ( dataPtr->laus_var_data.syscallData.result != NO_RETURN_CODE ) &&
		 ( ( ( dataPtr->laus_var_data.syscallData.result == -1 ) &&
		     ( -sycl->result !=
		       dataPtr->laus_var_data.syscallData.resultErrno ) ) 
		   || ( ( dataPtr->laus_var_data.syscallData.result != -1 ) && 
			( sycl->result != 
			  dataPtr->laus_var_data.syscallData.result ) ) ) ) {
		printf9("Result and/or error code didn't match:\n");
		printf9("\tBegin Range: %s", ctime(&(dataPtr->begin_r_time)));
		printf9("\tEnd   Range: %s", ctime(&(dataPtr->end_r_time)));
		printf9("Message:    %s", ctime(&(r_time)));
		printf9( "\tactual result:expected result:expected errno\t%i:%i:%i\n", 
			 sycl->result, dataPtr->laus_var_data.syscallData.result,
			 dataPtr->laus_var_data.syscallData.resultErrno );
		continue;
	    }

	    //message not a match if: (variable args)
	    if ( compare_var_parms( sycl->data, 
				    dataPtr->laus_var_data.syscallData.data 
				    ) != 0 ) {
		printf9("Variable arguments didn't match\n");
		printf9("\tActual:\n");
		var_parms( 9, sycl->major, sycl->data );
		printf9("\tExpected:\n");
		var_parms( 9, dataPtr->laus_var_data.syscallData.code, 
			   dataPtr->laus_var_data.syscallData.data );
		continue;
	    }
	    printf2("Message match found for %s in round %d\n", dataPtr->testName, count);
	    printf8("\tBegin Range: %s", ctime(&(dataPtr->begin_r_time)));
	    printf8("\tEnd   Range: %s", ctime(&(dataPtr->end_r_time)));
	    printf8("Message:  %s", ctime(&(r_time)));
	    printf8("Seqnr: %d\n", msg->msg_seqnr); 
	    var_parms( 8, dataPtr->laus_var_data.syscallData.code,
		       dataPtr->laus_var_data.syscallData.data );
	    msg_info( 8, msg, dataPtr );
	    sycl_info( 8, sycl, dataPtr );
	    printf8("\tresult     : %d\n", sycl->result);
	    break;
	case AUDIT_MSG_TEXT:
	    
	    if ( dataPtr->msg_evname[0] != '\0' && ( rc = strcmp ( msg->msg_evname, dataPtr->msg_evname )) != 0 ){
		printf9("event name didn't match\n");
		printf9("\tActual  :%s\n\tExpected: %s\n", msg->msg_evname, dataPtr->msg_evname );
		continue;
	    }
	    len = msg->msg_size - sizeof(struct aud_message_on_disk);
	    usrdata = (char *)&msg->msg_data;
	    if ( (rc = strncmp( usrdata,
				dataPtr->laus_var_data.textData.data,
				len ))
		 != 0 ) {
		printf9("audit text didn't match\n");
		printf9("\tActual  : %.*s\n\t\tExpected: %s\n", len, usrdata,
			dataPtr->laus_var_data.textData.data);
		continue;
	    }

	    printf2("Message match found in round %d\n", count);
	    printf8("\tBegin Range: %s", ctime(&(dataPtr->begin_r_time)));
	    printf8("\tEnd   Range: %s", ctime(&(dataPtr->end_r_time)));
	    printf8("Message:  %s", ctime(&(r_time)));
	    printf8("Segnr: %d\n", msg->msg_seqnr);
	    printf8("Text: %.*s\n", len, usrdata);
	    msg_info( 8, msg, dataPtr );
	    break;
	case AUDIT_MSG_LOGIN:
	    login = (struct aud_msg_login *)&msg->msg_data;
	    if ( login->uid != dataPtr->laus_var_data.loginData.uid ||
		 strcmp ( login->hostname, 
			  dataPtr->laus_var_data.loginData.hostname 
			  ) != 0 ||
		 strcmp ( login->address, 
			  dataPtr->laus_var_data.loginData.address
			  ) != 0 ||
		 strcmp ( login->terminal, 
			  dataPtr->laus_var_data.loginData.terminal
			  ) != 0 ||
		 strcmp ( login->executable,
			  dataPtr->laus_var_data.loginData.executable
			  ) != 0
		 ) {
		printf9("login data didn't match\n");
		printf9("Field:      Actual\tExpected:\n");
		printf9("uid:        %d\t%d\n", login->uid, dataPtr->laus_var_data.loginData.uid );
		printf9("hostname:   %s\t%s\n", login->hostname, dataPtr->laus_var_data.loginData.hostname );
		printf9("address:    %s\t%s\n", login->address, dataPtr->laus_var_data.loginData.address );
		printf9("terminal:   %s\t%s\n", login->terminal, dataPtr->laus_var_data.loginData.terminal );
		printf9("executable: %s\t%s\n", login->executable, dataPtr->laus_var_data.loginData.executable );
		continue;
	    }
	    printf2("Message match found in round %d\n", count);
	    printf8("\tBegin Range: %s", ctime(&(dataPtr->begin_r_time)));
	    printf8("\tEnd   Range: %s", ctime(&(dataPtr->end_r_time)));
	    printf8("Message:  %s", ctime(&(r_time)));
	    printf8("Seqnr: %d\n", msg->msg_seqnr);
	    printf8("uid:        %d\n", login->uid);
	    printf8("hostname:   %s\n", login->hostname);
	    printf8("address:    %s\n", login->address);
	    printf8("terminal:   %s\n", login->terminal);
	    printf8("executable: %s\n", login->executable);
	    break;
	case AUDIT_MSG_EXIT:
	    exit = (struct aud_msg_exit_on_disk *)&msg->msg_data;
	    if (exit->code != dataPtr->laus_var_data.exitData.code ) {
		printf9("audit exit code didn't match\n");
		printf9("\tActual  : %li\n\t\tExpected: %li\n",
			exit->code, dataPtr->laus_var_data.exitData.code);
		continue;
	    }

	    printf2("Message match found in round %d\n", count);
	    printf8("\tBegin Range: %s", ctime(&(dataPtr->begin_r_time)));
	    printf8("\tEnd   Range: %s", ctime(&(dataPtr->end_r_time)));
	    printf8("Message:  %s", ctime(&(r_time)));
	    printf8("Exit Code: %li\n",exit->code);
	    msg_info( 8, msg, dataPtr );
	    break;
	case AUDIT_MSG_NETLINK:
	    netlink = (struct aud_msg_netlink *)&msg->msg_data;
	    if (netlink->result != dataPtr->laus_var_data.netlinkData.result ) {
		printf9("audit netlink result didn't match\n");
		printf9("\tActual  : %d\n\t\tExpected: %d\n",
			netlink->result, dataPtr->laus_var_data.netlinkData.result);
		continue;
	    }

	    printf2("Message match found in round %d\n", count);
	    printf8("\tBegin Range: %s", ctime(&(dataPtr->begin_r_time)));
	    printf8("\tEnd   Range: %s", ctime(&(dataPtr->end_r_time)));
	    printf8("Message:  %s", ctime(&(r_time)));
	    printf8("Seqnr: %d\n", msg->msg_seqnr);
	    printf8("Result: %d\n", netlink->result);
	    msg_info( 8, msg, dataPtr );
	    break;
	default:
	    printf1("ERROR: potentially matching message found but this function can currently only handle system call, text, login, exit and netlink message types\n");
	    goto OUT;
	    break;
	}

	//we have a match
	record_found = TRUE;
	break;

    }
     //This is for messages which weren't in the log yet
    if ( msg == NULL ) {
    //AUDIT_RETRY:              // not needed?
	
	if ( ( logOption.logSuccess && dataPtr->successCase )
	     || ( logOption.logFailure && !dataPtr->successCase) ) {  
	    count ++;
	    if ( count > 10 ) {
		printf1("ERROR: Message for %s not found in the log\n", dataPtr->testName );
	    }
	    else {
		close_audit_log(&log);
#ifndef NOSLEEP
		sleep(4);
#endif
		goto AUDIT_BEGIN;
	    }
	}
    }

    if ( (dataPtr->successCase && logOption.logSuccess) || 
	 (!dataPtr->successCase && logOption.logFailure) ) {
	printf2("Verify record\n");
	if ( record_found == 1) {
	    pass_testcases++;
	    printf2("AUDIT PASS ");
	}
	else {
	    fail_testcases++;
	    debug_expected(dataPtr);
	    printf2("AUDIT FAIL ");
	}
	
	printf2prime(": '%s' [logSuccess=%x, logFailure=%x, successCase=%x]\n",
		     dataPtr->testName, logOption.logSuccess, 
		     logOption.logFailure, dataPtr->successCase);
	
    } else {
	printf2("Verify no record\n");
	if ( record_found == 0 ) {
	    pass_testcases++;
	    printf2("AUDIT PASS ");
	}
	else {
	    fail_testcases++;
	    printf2("AUDIT FAIL ");
	}
	
	printf2prime(": '%s' [logSuccess=%x, logFailure=%x, successCase=%x]\n",
		     dataPtr->testName, logOption.logSuccess, 
		     logOption.logFailure, dataPtr->successCase);
    } 
    
    
 OUT:
    //REAL AUDIT VERIFY CLEANUP
    close_audit_log(&log);
    
 OUT_NOTOPEN:
    return 0;
}

/**********************
 ** HELPER FUNCTIONS **
 *********************/

/** OUTPUT TYPE **/

void msg_info (int level, const struct aud_message_on_disk* msg, const laus_data* dataPtr)
{
  printf_level(level, "\ttype  : %i:%i\n", msg->msg_type, dataPtr->msg_type);
  printf_level(level, "\tarch  : %i:%i\n", msg->msg_arch, dataPtr->msg_arch);
  printf_level(level, "\tpid   : %i:%i\n", msg->msg_pid, dataPtr->msg_pid);
  printf_level(level, "\tluid  : %i:%i\n", msg->msg_login_uid, dataPtr->msg_login_uid);
  printf_level(level, "\teuid  : %i:%i\n", msg->msg_euid, dataPtr->msg_euid);
  printf_level(level, "\truid  : %i:%i\n", msg->msg_ruid, dataPtr->msg_ruid);
  printf_level(level, "\tsuid  : %i:%i\n", msg->msg_suid, dataPtr->msg_suid);
  printf_level(level, "\tfsuid : %i:%i\n", msg->msg_fsuid, dataPtr->msg_fsuid);
  printf_level(level, "\tegid  : %i:%i\n", msg->msg_egid, dataPtr->msg_egid);
  printf_level(level, "\trgid  : %i:%i\n", msg->msg_rgid, dataPtr->msg_rgid);
  printf_level(level, "\tsgid  : %i:%i\n", msg->msg_sgid, dataPtr->msg_sgid);
  printf_level(level, "\tfsgid : %i:%i\n", msg->msg_fsgid, dataPtr->msg_fsgid);
}

void sycl_info(int level, const struct aud_msg_syscall * sycl, const laus_data* dataPtr)
{
    printf_level(level, "\tmajor  : %i:%i\n", sycl->major, 
		 dataPtr->laus_var_data.syscallData.code);
    printf_level(level, "\tminor  : %i:%i\n", sycl->minor, 
		 dataPtr->laus_var_data.syscallData.minor);
    printf_level(level, "\tlength : %i:%i\n", sycl->length, 
		 dataPtr->laus_var_data.syscallData.length);    
}

void var_parms (int level, int call, const char *src )
{    
    char* buffer  = NULL;
    char* vector_buffer = NULL;
    int buffer_index = 0;
    int arg_size = 0;
    ARG_TYPE arg_type = AUDIT_ARG_STRING;
    long long tmpint    = 0;
    ARG_TYPE type = AUDIT_ARG_NULL;
    int size      = 0;
    int rc;

    if (src != NULL) {
	buffer = (char*)malloc(BUFSIZE);
	memset(buffer, '\0', BUFSIZE);

	while ((rc = arg_get(&type, &size, buffer, &src)) != -1) {
	    switch ( type ) { 
        case AUDIT_ARG_VECTOR:
        printf_level(level, "\t\ttype = %i, size = %i\n", type, size);
        vector_buffer = malloc(size);
        while (buffer_index < size) {
            memcpy((char *)&arg_type, buffer + buffer_index, sizeof(type));
            buffer_index += sizeof(type);
            memcpy((char *)&arg_size, buffer + buffer_index, sizeof(size));
            buffer_index += sizeof(size);
            memset(vector_buffer, '\0', size);
            memcpy(vector_buffer, buffer + buffer_index, arg_size);
            printf_level(level, "\t\t\ttype = %i, size = %i, data = %s\n", 
                        arg_type, arg_size, vector_buffer);
            buffer_index += arg_size;
        }
        break;
        case AUDIT_ARG_NULL:
		printf_level(level, "\t\ttype = %i, size = %i, data = NULL\n",
			     type, size);
		break;
	    case AUDIT_ARG_ERROR:
	    case AUDIT_ARG_IMMEDIATE:
		memcpy(&tmpint, buffer, size);
		tmpint = (long) tmpint;
		printf_level(level, "\t\ttype = %i, size = %i, data = %Ld\n",
			     type, size, tmpint);
		break;
	    case AUDIT_ARG_POINTER: 
		printf_level(level, "\t\ttype = %i, size = %i, ", type, size);
		int idx; //array index, set to start in each case
		int bnd; //array bounds, total num elements set in each case
		void * grp_array = NULL;

		switch (call) {
#ifndef NOXATTR 
		case __NR_setxattr:
		case __NR_lsetxattr:
		case __NR_fsetxattr:
#endif
		case __NR_sethostname:
		case __NR_setdomainname:
		    bnd = size/sizeof(char);
		    grp_array = (void*)malloc(size);
		    memcpy( grp_array, buffer, size );
		    printf_level( level, "data=%.*s", bnd, ((char*)grp_array) );
		    break;
#if !defined(__PPC64) && !defined(__X86_64)
		case __NR_setgroups:
		    bnd = size/sizeof(u_int16_t);
		    idx = 0;
		    grp_array = (void*)malloc(size);
		    memcpy( grp_array, buffer, size );
		    for ( idx = 0; idx < bnd; idx++ ) 
			printf_level_prime( level, "\n\t\t\tindex=%i, value=%i",
					    idx, ((u_int16_t*)grp_array)[idx] );
		    break;
#if !defined(__PPC) && !defined(__S390X) && !defined(__IA64)
		case __NR_setgroups32:

		    bnd = size/sizeof(gid_t);
		    idx = 0;
		    grp_array = (void*)malloc(size);
		    memcpy( grp_array, buffer, size );
		    for ( idx = 0; idx < bnd; idx++ )
			printf_level_prime( level, "\n\t\t\tindex=%i, value=%i",
					    idx, ((gid_t*)grp_array)[idx] );
		    break;
#endif
#else
		case __NR_setgroups:
		    bnd = size/sizeof(gid_t);
		    idx = 0;
		    grp_array = (void*)malloc(size);
		    memcpy( grp_array, buffer, size );
		    for ( idx = 0; idx < bnd; idx++ )
			printf_level_prime( level, "\n\t\t\tindex=%i, value=%i",
					    idx, ((gid_t*)grp_array)[idx] );
		    break;
#endif		    
		default:
		    memcpy(&tmpint, buffer, sizeof(tmpint));
		    tmpint = (long) tmpint;
		    printf_level_prime(level, "data = %Lu", tmpint);
		} 
		printf_level_prime( level, "\n");
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

void debug_expected( const laus_data* dataPtr)
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
	printf5("\tresult    : %i\n", dataPtr->laus_var_data.syscallData.result);
	printf5("\terrno     : %i\n", dataPtr->laus_var_data.syscallData.resultErrno);
	
	//Dump syscall parameters
	printf5("\tparameters\n");
	var_parms(5, dataPtr->laus_var_data.syscallData.code, 
		  dataPtr->laus_var_data.syscallData.data);
	
	// Dump user space audit data
    } else if (dataPtr->msg_type == AUDIT_MSG_TEXT) {
	printf5("\ttext: %s\n", dataPtr->laus_var_data.textData.data);
    }
}

/** COMPARATOR TYPE **/

int compare_var_parms (const char *actual, const char *expect)
{    
    char* buffer_a  = NULL;
    char* buffer_e  = NULL;
    //long long tmplong_a    = 0;    // variables not needed?
    //long long tmplong_e    = 0;
    //int tmpint_a    = 0;
    //int tmpint_e    = 0;

    ARG_TYPE type_a = AUDIT_ARG_NULL;
    ARG_TYPE type_e = AUDIT_ARG_NULL;
    int size_a      = 0;
    int size_e      = 0;
    int rc_a, rc_e, rc = 0;
    int i = 0;
    
    if (actual == NULL && expect == NULL) { //both empty match
	printf9("compare_var_parms: actual and expect are NULL\n");
	return ( 0 );
	
    }
    
    if (actual == NULL || expect == NULL) { //one empty no match
	printf1("Insufficient data actual ptr: %p, expected ptr: %p\n",
		actual, expect);
	if (actual == NULL) {
		printf9("compare_var_parms: actual is NULL\n");
	} else {
		printf9("compare_var_parms: expect is NULL\n");
	}
	return ( -1 ); 
    }
    
    buffer_a =  (char*)malloc(BUFSIZE);
    memset(buffer_a, '\0', BUFSIZE);

    buffer_e =  (char*)malloc(BUFSIZE);
    memset(buffer_e, '\0', BUFSIZE);

    while ( ((rc_a = arg_get(&type_a, &size_a, buffer_a, &actual)) != -1) &&
	    ((rc_e = arg_get(&type_e, &size_e, buffer_e, &expect)) != -1) ) {

	if (type_e == AUDIT_ARG_IGNORE) {
          printf9("skip arg check\n");
	  continue;
        }

	if ((type_a == AUDIT_ARG_STRING) || (type_a == AUDIT_ARG_PATH)) {
		printf9("compare_var_parms: actual type=%d size=%d data=%s\n", type_a, size_a, buffer_a);
	} else {
		printf9("compare_var_parms: actual type=%d size=%d\n", type_a, size_a);
	}
        if ((type_e == AUDIT_ARG_STRING) || (type_e == AUDIT_ARG_PATH)) {
		printf9("compare_var_parms: expect type=%d size=%d data=%s\n", type_e, size_e, buffer_e);
        } else {
		printf9("compare_var_parms: expect type=%d size=%d\n", type_e, size_e);
        }

	if ( type_a != type_e  || size_a != size_e  || size_a < 0 ) {
	    printf9("Size or type mismatch in variable arguments\n");
	    rc = -1;
	    goto out;
	} else {
            if ( memcmp( buffer_a, buffer_e, size_e ) != 0 ) {
		printf9("String value mismatch in variable arguments\n");
		printf9prime("actual: ");
		for (i = 0; i < size_a; i++) {
			printf("%02X", buffer_a[i]);
		}
		printf9prime("\n");
                printf9prime("expect: ");
                for (i = 0; i < size_e; i++) {
                        printf("%02X", buffer_e[i]);
                }
                printf9prime("\n");
		rc = -1;
		goto out;
	    }
	}
	
	//Clean buffers inside each loop iteration before doing next arg_get
	memset(buffer_a, '\0', BUFSIZE);
	memset(buffer_e, '\0', BUFSIZE);
    }
    
 out:
    free(buffer_a);
    free(buffer_e);
    return rc;
}

int verify(int return_code, laus_data* dataPtr, log_options logOption) {
  int rc = 0;

  switch ( return_code ) {
    case 0:
	//verifyLog will update pass and fail testcase count
      if ((rc = verifyLog(dataPtr, logOption)) != 0) {
        goto EXIT_ERROR;
      }
      break;
    case SKIP_TEST_CASE:
	skip_testcases++;

	printf2("%s() test case skipped: [logSuccess=%d, logFailure=%d, successCase=%d]\n", 
		dataPtr->testName, logOption.logSuccess, 
		logOption.logFailure, dataPtr->successCase);
			       
      break;
    default:
	fail_testcases++;

	printf1("ERROR: %s() test invalid: [logSuccess=%d, logFailure=%d, successCase=%d]\n", 
		dataPtr->testName, logOption.logSuccess, 
		logOption.logFailure, dataPtr->successCase);
	printf2("AUDIT FAIL : '%s' test invalid: [%s, logSuccess=%d, logFailure=%d]\n", 
		dataPtr->testName, dataPtr->successCase ? "successCase" : "failureCase",
		logOption.logSuccess, logOption.logFailure);
      break;
  }

 EXIT_ERROR:
  if (dataPtr->laus_var_data.syscallData.data)
      free(dataPtr->laus_var_data.syscallData.data);
  
  return rc;

}


 
