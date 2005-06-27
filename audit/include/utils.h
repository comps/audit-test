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

int size_of_file(char*);
int backupFile(char*);
int restoreFile(char*);
int getLAUSData(laus_data*);
int getLoginUID();
int clearAuditTrail();
int setFilterDomain(log_options logOption);
int createFile(char* fname, mode_t mode, uid_t uid, gid_t gid);
int createTempFile(char** fname, mode_t mode, uid_t uid, gid_t gid);
int createTempFileName(char** fname);
int createTempDir(char** fname, mode_t mode, uid_t uid, gid_t gid);
int createTempUser(char** user, int* uid, char** homedir);
int createTempUserName(char** user, int* uid, char** homedir);
int createTempGroupName( char** user, int* uid );
int startAudit();
int stopAudit();
int reloadAudit();
int stopClearStartAudit();
int verifyLog(laus_data* dataPtr, log_options logOption);
int verify(int return_code, laus_data* dataPtr, log_options logOption);
int getIdentifiers( identifiers_t* identifiers );
int getPid(char* executable);
int preSysCall(laus_data* dataPtr);
int postSysCall(laus_data* dataPtr, int resultErrno, int errorRC, int expectedErrno);
int arg_vector(char** vector, int vector_size, const ARG_TYPE type,
	const int size, const char* src);
int arg_get(ARG_TYPE* ptype, int* psize, char* dest, const char** src);
int auditArg0( laus_data* lausDataPtr );
int auditArg1( laus_data* lausDataPtr, const int auditArgType, const int size, void* dataPtr );
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

char* mysprintf(char* fmt, ...);
int run(char* command); 

#endif
