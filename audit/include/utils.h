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
**  FILE   : utils.h
**
**  PURPOSE: This file contains the function declarations for each of
**           the utility functions as defined in their own source code
**           files in ../utils
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**    07/03 furthered by Michael A. Halcrow <mike@halcrow.us>
**    08/03 furthered by Daniel H. Jones <danjones@us.ibm.com>
**
**********************************************************************/

#ifndef _UTILS_H
#define _UTILS_H

#include "testsuite.h" /* for log_options */

/*
 * Audit Interaction
 */
int audit_start();
int audit_stop();
int audit_clear_logs();
int audit_reload();
int audit_set_filters(log_options);
int audit_verify_log(laus_data *, log_options);

/*
 * System Info Collection
 */
typedef struct identifiers_s {
    int ruid;
    int euid;
    int suid;
    int fsuid;
    int rgid;
    int egid;
    int sgid;
    int fsgid;
} identifiers_t;

int getLAUSData(laus_data*);
int getLoginUID();
int getIdentifiers( identifiers_t* identifiers );
int getPid(char* executable);

/*
 * File Content Preservation
 */
int backupFile(char*);
int restoreFile(char*);

/*
 * Temp Data Creation
 */
int createTempFile(char** fname, mode_t mode, uid_t uid, gid_t gid);
int createTempFileName(char** fname);
int createTempDir(char** fname, mode_t mode, uid_t uid, gid_t gid);
int createTempUser(char** user, int* uid, char** homedir);
int createTempUserName(char** user, int* uid, char** homedir);
int createTempGroupName( char** user, int* uid );

/*
 * Syscall Args Encoding
 */
int arg_get(ARG_TYPE* ptype, int* psize, char* dest, const char** src);
int arg_vector(char** vector, int vector_size, 
	       const ARG_TYPE type, const int size, const char* src);
int auditArg0( laus_data* lausDataPtr );
int auditArg1( laus_data* lausDataPtr, 
	       const int auditArgType, const int size, void* dataPtr );
int auditArg2( laus_data* lausDataPtr,
	       const int auditArgType1, const int size1, void* dataPtr1,
	       const int auditArgType2, const int size2, void* dataPtr2 );
int auditArg3( laus_data* lausDataPtr,
	       const int auditArgType1, const int size1, void* dataPtr1,
	       const int auditArgType2, const int size2, void* dataPtr2,
	       const int auditArgType3, const int size3, void* dataPtr3 );
int auditArg4( laus_data* lausDataPtr,
	       const int auditArgType1, const int size1, void* dataPtr1,
	       const int auditArgType2, const int size2, void* dataPtr2,
	       const int auditArgType3, const int size3, void* dataPtr3,
	       const int auditArgType4, const int size4, void* dataPtr4 );
int auditArg5( laus_data* lausDataPtr,
	       const int auditArgType1, const int size1, void* dataPtr1,
	       const int auditArgType2, const int size2, void* dataPtr2,
	       const int auditArgType3, const int size3, void* dataPtr3,
	       const int auditArgType4, const int size4, void* dataPtr4,
	       const int auditArgType5, const int size5, void* dataPtr5 );

/*
 * Syscall Test Setup/Teardown
 */
int preSysCall(laus_data* dataPtr);
int postSysCall(laus_data* dataPtr, int resultErrno, int errorRC, 
		int expectedErrno);

/*
 * Shell Command Utilities
 */
int run(char* command); 
char* mysprintf(char* fmt, ...);
#define RUNCOMMAND(cmd, ...) \
do {								\
    char* command = mysprintf(cmd, ## __VA_ARGS__);		\
    if ((rc = system(command)) == -1) {				\
	printf1("Error running command: [%s]\n", command);	\
    }								\
    free(command);						\
} while (0)							\

#define RUNCOMMANDORDIE(cmd, ...) \
do {								\
    char* command = mysprintf(cmd, ## __VA_ARGS__ );		\
    if ((rc = system(command)) == -1) {				\
	printf1("Error running command: [%s]\n", command);	\
	free(command);						\
	goto EXIT_CLEANUP;					\
    }								\
    free(command);						\
} while (0)

/*
 * Debug Messages
 */
void debug_expected(const laus_data* dataPtr);

#define printf1(msg, ...)		\
do {					\
    if (debug >= 1) {			\
	printf( "ENVIRONMENT ");	\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf2(msg, ...)		\
do {					\
    if (debug >= 2) {			\
	printf( "TEST ");		\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf2prime(msg, ...)		\
do {					\
    if (debug >= 2) {			\
	printf(msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf3(msg, ...)		\
do {					\
    if (debug >= 3) {			\
	printf( "WARNING ");		\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf4(msg, ...)		\
do {					\
    if (debug >= 4) {			\
	printf( "INFO ");		\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf5(msg, ...)		\
do {					\
    if (debug >= 5) {			\
	printf( "DEBUG ");		\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf5prime(msg, ...)		\
do {					\
    if (debug >= 5) {			\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf8(msg, ...)		\
do {					\
    if (debug >= 8) {			\
	printf( "AUDIT ");		\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf8prime(msg, ...)		\
do {					\
    if (debug >= 8) {			\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf9(msg, ...)		\
do {					\
    if (debug >= 9) {			\
	printf( "AUDIT DEBUG ");	\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf9prime(msg, ...)		\
do {					\
    if (debug >= 9) {			\
	printf( msg, ## __VA_ARGS__);	\
    }					\
} while (0)

#define printf_level(lev, msg, ...)	\
do {					\
    switch (lev) {			\
    case 1:				\
	printf1(msg, ## __VA_ARGS__);	\
	break;				\
    case 2:				\
	printf2(msg, ## __VA_ARGS__);	\
	break;				\
    case 3:				\
	printf3(msg, ## __VA_ARGS__);	\
	break;				\
    case 4:				\
	printf4(msg, ## __VA_ARGS__);	\
	break;				\
    case 5:				\
	printf5(msg, ## __VA_ARGS__);	\
	break;				\
    case 8:				\
	printf8(msg, ## __VA_ARGS__);	\
	break;				\
    case 9:				\
	printf9(msg, ## __VA_ARGS__);	\
	break;				\
    }					\
} while (0)

#define printf_level_prime(lev, msg, ...)	\
do {						\
    switch (lev) {				\
    case 2:					\
	printf2prime(msg, ## __VA_ARGS__);	\
	break;					\
    case 5:					\
	printf5prime(msg, ## __VA_ARGS__);	\
	break;					\
    case 8:					\
	printf8prime(msg, ## __VA_ARGS__);	\
	break;					\
    case 9:					\
	printf9prime(msg, ## __VA_ARGS__);	\
	break;					\
    }						\
} while (0)

#endif	/* _UTILS_H */
