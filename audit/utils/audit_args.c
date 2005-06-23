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
**  FILE   : audit_args.c
**
**  PURPOSE: In analyzing the results of both successful and erroneous
**           system calls, it is useful to record and later retrieve
**           the arguments passed to the system call.  The functions
**           defined within this file provide an interface to
**           accomplish these tasks.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**    05/04 Removed const from type parameter for arg_put by Kimberly D.
**          Simon <kdsimon@us.ibm.com>
**    05/04 Updates to suppress compile warnings by Kimberly D. Simon <kdsimon@us.ibm.com>   
**
**********************************************************************/

#include "utils.h"
#include "types.h"


/*
** This function is used to build the syscall parameters data. The first call (with a null pointer)
** allocates the buffer space. Each subsequent call expands the buffer and appends the syscall data.
** 
** The function will also map "immediate" and "error" type parameters to a 64 bit data type and 
** adjust the size accordingly.
*/

int arg_put( laus_data* lausDataPtr, ARG_TYPE type, const int size, char* src) {

    int newsize = size;
    char*newsrc = src;
    __laus_int64 recast64;

    if ((type == AUDIT_ARG_IMMEDIATE) && (size != sizeof(__laus_int64))) {
	// sign extend 32bit signed int to signed 64bit
	// Example: ffffffff => ffffffffffffffff
	recast64 = *(__s32 *)src;
	newsize = sizeof(recast64);
	newsrc = (char*)&recast64;
    } else if ((type == AUDIT_ARG_IMMEDIATE_u) && (size != sizeof(__laus_int64))) {
        // convert unsigned 32bit to signed 64bit
        // Example: ffffffff => 00000000ffffffff
        recast64 = *(__u32 *)src;
        newsize = sizeof(recast64);
        newsrc = (char*)&recast64;
        type = AUDIT_ARG_IMMEDIATE;
    }

    lausDataPtr->laus_var_data.syscallData.data = 
      realloc(lausDataPtr->laus_var_data.syscallData.data, lausDataPtr->laus_var_data.syscallData.length + sizeof(type));
    memcpy(lausDataPtr->laus_var_data.syscallData.data + lausDataPtr->laus_var_data.syscallData.length, &type, sizeof(type));
    lausDataPtr->laus_var_data.syscallData.length += sizeof(type);
    
    if (type != AUDIT_ARG_END){
	
        lausDataPtr->laus_var_data.syscallData.data = 
	  realloc(lausDataPtr->laus_var_data.syscallData.data, lausDataPtr->laus_var_data.syscallData.length + sizeof(newsize));
	memcpy(lausDataPtr->laus_var_data.syscallData.data + lausDataPtr->laus_var_data.syscallData.length, &newsize, sizeof(newsize));
	lausDataPtr->laus_var_data.syscallData.length += sizeof(newsize);
	
	if (size != 0) {
            
	    lausDataPtr->laus_var_data.syscallData.data = 
	      realloc(lausDataPtr->laus_var_data.syscallData.data, lausDataPtr->laus_var_data.syscallData.length + newsize);
	    memcpy(lausDataPtr->laus_var_data.syscallData.data + lausDataPtr->laus_var_data.syscallData.length, newsrc, newsize);
	    lausDataPtr->laus_var_data.syscallData.length += newsize;
	}
    }
    return 0;
}

/*
** Function will concatenate type, length, data to a buffer and
** return the new buffer length.
*/

int arg_vector( char** vector, int vector_size, const ARG_TYPE type, const int size, const char* src) {

    int new_length = vector_size + sizeof(type) + sizeof(size) + size;
    char * vector_index;

    *vector = realloc(*vector, new_length);
    vector_index = *vector + vector_size;
    memcpy(vector_index, &type, sizeof(type));
    vector_index += sizeof(type);
    memcpy(vector_index, &size, sizeof(size));
    vector_index += sizeof(size);
    memcpy(vector_index, src, size);

    return new_length;
}

/*
** Function will return the type, size, and parameter values
** from the source buffer and reset the source buffer pointer
** to the next position.
*/
int arg_get(ARG_TYPE* ptype, int* psize, char* dest, const char** src) {
  int rc = 0;
  memcpy(ptype, *src, sizeof(*ptype));
  *src += sizeof(*ptype);
  if (*ptype == AUDIT_ARG_END) {
    rc = -1;
    goto EXIT;
  }
  memcpy(psize, *src, sizeof(*psize));
  *src += sizeof(*psize);
  
  if (!(*ptype == AUDIT_ARG_NULL)) {
      memcpy(dest, *src, *psize);
      *src += *psize;
  }
 EXIT:
  return rc;
}

int auditArgNil( laus_data* lausDataPtr ) {

  lausDataPtr->laus_var_data.syscallData.personality = NO_CHECK_SYSCALL_DATA;
  lausDataPtr->laus_var_data.syscallData.data = NULL;
  lausDataPtr->laus_var_data.syscallData.length = 0;
  arg_put( lausDataPtr, AUDIT_ARG_END, 0, NULL );    
  //lausDataPtr->laus_var_data.syscallData.length += sizeof(char *);
  return 0;
}

int auditArg0( laus_data* lausDataPtr ) {

  lausDataPtr->laus_var_data.syscallData.data = NULL;
  lausDataPtr->laus_var_data.syscallData.length = 0;
  arg_put( lausDataPtr, AUDIT_ARG_END, 0, NULL );    
  lausDataPtr->laus_var_data.syscallData.length += 4;
  return 0;
}

int auditArg1( laus_data* lausDataPtr, 
	       const int auditArgType, const int size, void* dataPtr ) {

  if( (dataPtr == NULL) && ((auditArgType != AUDIT_ARG_NULL) && (auditArgType != AUDIT_ARG_VECTOR)) ) {
    printf1( "Attempt to add NULL pointer to audit args buffer, but auditArgType != AUDIT_ARG_NULL\n" );
    return 1;
  }
  lausDataPtr->laus_var_data.syscallData.data = NULL;
  lausDataPtr->laus_var_data.syscallData.length = 0;
  arg_put( lausDataPtr, auditArgType, size, dataPtr );
  arg_put( lausDataPtr, AUDIT_ARG_END, 0, NULL );    
  lausDataPtr->laus_var_data.syscallData.length += 4;
  return 0;
}

int auditArg2( laus_data* lausDataPtr,
	       const int auditArgType1, const int size1, void* dataPtr1,
	       const int auditArgType2, const int size2, void* dataPtr2 ) {

  if( ( (dataPtr1 == NULL) && ((auditArgType1 != AUDIT_ARG_NULL) && (auditArgType1 != AUDIT_ARG_VECTOR)) ) || 
      ( (dataPtr2 == NULL) && ((auditArgType2 != AUDIT_ARG_NULL) && (auditArgType2 != AUDIT_ARG_VECTOR)) ) ) {
    printf1( "Attempt to add NULL pointer to audit args buffer, but auditArgType != AUDIT_ARG_NULL\n" );
    return 1;
  }
  lausDataPtr->laus_var_data.syscallData.data = NULL;
  lausDataPtr->laus_var_data.syscallData.length = 0;
  arg_put( lausDataPtr, auditArgType1, size1, dataPtr1 );
  arg_put( lausDataPtr, auditArgType2, size2, dataPtr2 );
  arg_put( lausDataPtr, AUDIT_ARG_END, 0, NULL );    
  lausDataPtr->laus_var_data.syscallData.length += 4;
  return 0;
}

int auditArg3( laus_data* lausDataPtr,
	       const int auditArgType1, const int size1, void* dataPtr1,
	       const int auditArgType2, const int size2, void* dataPtr2,
	       const int auditArgType3, const int size3, void* dataPtr3 ) {

  if( ( (dataPtr1 == NULL) && ((auditArgType1 != AUDIT_ARG_NULL) && (auditArgType1 != AUDIT_ARG_VECTOR)) ) || 
      ( (dataPtr2 == NULL) && ((auditArgType2 != AUDIT_ARG_NULL) && (auditArgType2 != AUDIT_ARG_VECTOR)) ) ||
      ( (dataPtr3 == NULL) && ((auditArgType3 != AUDIT_ARG_NULL) && (auditArgType3 != AUDIT_ARG_VECTOR)) ) ) {
    printf1( "Attempt to add NULL pointer to audit args buffer, but auditArgType != AUDIT_ARG_NULL\n" );
    return 1;
  }
  lausDataPtr->laus_var_data.syscallData.data = NULL;
  lausDataPtr->laus_var_data.syscallData.length = 0;
  arg_put( lausDataPtr, auditArgType1, size1, dataPtr1 );
  arg_put( lausDataPtr, auditArgType2, size2, dataPtr2 );
  arg_put( lausDataPtr, auditArgType3, size3, dataPtr3 );
  arg_put( lausDataPtr, AUDIT_ARG_END, 0, NULL );    
  lausDataPtr->laus_var_data.syscallData.length += 4;
  return 0;
}

int auditArg4( laus_data* lausDataPtr,
	       const int auditArgType1, const int size1, void* dataPtr1,
	       const int auditArgType2, const int size2, void* dataPtr2,
	       const int auditArgType3, const int size3, void* dataPtr3,
	       const int auditArgType4, const int size4, void* dataPtr4 ) {

  if( ( (dataPtr1 == NULL) && ((auditArgType1 != AUDIT_ARG_NULL) && (auditArgType1 != AUDIT_ARG_VECTOR)) ) || 
      ( (dataPtr2 == NULL) && ((auditArgType2 != AUDIT_ARG_NULL) && (auditArgType2 != AUDIT_ARG_VECTOR)) ) ||
      ( (dataPtr3 == NULL) && ((auditArgType3 != AUDIT_ARG_NULL) && (auditArgType3 != AUDIT_ARG_VECTOR)) ) ||
      ( (dataPtr4 == NULL) && ((auditArgType4 != AUDIT_ARG_NULL) && (auditArgType4 != AUDIT_ARG_VECTOR)) ) ) {
    printf1( "Attempt to add NULL pointer to audit args buffer, but auditArgType != AUDIT_ARG_NULL\n" );
    return 1;
  }
  lausDataPtr->laus_var_data.syscallData.data = NULL;
  lausDataPtr->laus_var_data.syscallData.length = 0;
  arg_put( lausDataPtr, auditArgType1, size1, dataPtr1 );
  arg_put( lausDataPtr, auditArgType2, size2, dataPtr2 );
  arg_put( lausDataPtr, auditArgType3, size3, dataPtr3 );
  arg_put( lausDataPtr, auditArgType4, size4, dataPtr4 );
  arg_put( lausDataPtr, AUDIT_ARG_END, 0, NULL );    
  lausDataPtr->laus_var_data.syscallData.length += 4;
  return 0;
}

int auditArg5( laus_data* lausDataPtr,
	       const int auditArgType1, const int size1, void* dataPtr1,
	       const int auditArgType2, const int size2, void* dataPtr2,
	       const int auditArgType3, const int size3, void* dataPtr3,
	       const int auditArgType4, const int size4, void* dataPtr4,
	       const int auditArgType5, const int size5, void* dataPtr5 ) {

  if( ( (dataPtr1 == NULL) && ((auditArgType1 != AUDIT_ARG_NULL) && (auditArgType1 != AUDIT_ARG_VECTOR)) ) || 
      ( (dataPtr2 == NULL) && ((auditArgType2 != AUDIT_ARG_NULL) && (auditArgType2 != AUDIT_ARG_VECTOR)) ) ||
      ( (dataPtr3 == NULL) && ((auditArgType3 != AUDIT_ARG_NULL) && (auditArgType3 != AUDIT_ARG_VECTOR)) ) ||
      ( (dataPtr4 == NULL) && ((auditArgType4 != AUDIT_ARG_NULL) && (auditArgType4 != AUDIT_ARG_VECTOR)) ) ||
      ( (dataPtr5 == NULL) && ((auditArgType5 != AUDIT_ARG_NULL) && (auditArgType5 != AUDIT_ARG_VECTOR)) ) ) {
    printf1( "Attempt to add NULL pointer to audit args buffer, but auditArgType != AUDIT_ARG_NULL\n" );
    return 1;
  }
  lausDataPtr->laus_var_data.syscallData.data = NULL;
  lausDataPtr->laus_var_data.syscallData.length = 0;
  arg_put( lausDataPtr, auditArgType1, size1, dataPtr1 );
  arg_put( lausDataPtr, auditArgType2, size2, dataPtr2 );
  arg_put( lausDataPtr, auditArgType3, size3, dataPtr3 );
  arg_put( lausDataPtr, auditArgType4, size4, dataPtr4 );
  arg_put( lausDataPtr, auditArgType5, size5, dataPtr5 );
  arg_put( lausDataPtr, AUDIT_ARG_END, 0, NULL );    
  lausDataPtr->laus_var_data.syscallData.length += 4;
  return 0;
}
