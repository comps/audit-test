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
**  FILE   : types.h
**
**  PURPOSE: This file contains the type definitions for all customized
**           data types used in the Linux Auditing System test suite.
**
**
**  HISTORY:
**    05/03 originated by Dan Jones (danjones@us.ibm.com)
**    06/03 furthered by Dustin Kirkland (k1rkland@us.ibm.com
**
**********************************************************************/

#ifndef _TYPES_H
#define _TYPES_H

#include "laus_info.h" /* temporarily needed for compilation */
#include <asm/types.h>

//
// New types
//
#if defined(__MODE_32) || defined(__IX86)
typedef long long __laus_int64;
typedef int __laus_int32;
#else
typedef long __laus_int64;
typedef int __laus_int32;
#endif

#define NO_CHECK_SYSCALL_DATA 0x7ba970a7

typedef struct {
	uid_t		uid;
	char		hostname[AUD_MAX_HOSTNAME];
	char		address[AUD_MAX_ADDRESS];
	char		terminal[AUD_MAX_TERMINAL];
        char		executable[PATH_MAX];
} laus_login ;

typedef struct {
        int             personality;
	int		code, minor;
	int		result;
        int             resultErrno;
        unsigned int    length;
	unsigned char*  data;	/* variable size */
} laus_syscall;

typedef struct {
        long code;
} laus_exit;

typedef struct {
        unsigned int    groups, dst_groups;
	int		result;
        unsigned int    length;
	unsigned char*  data;	/* variable size */
} laus_netlink;

typedef struct {
    unsigned char*  data;	/* variable size */
} laus_text;

typedef struct {
    char*        testName;
    Boolean      successCase;    
    time_t       begin_r_time;
    time_t       end_r_time;
    u_int32_t    msg_seqnr;
    u_int16_t    msg_type;
    u_int16_t    msg_arch;
    pid_t        msg_pid;
    size_t       msg_size;
    unsigned int msg_audit_id;
    unsigned int msg_login_uid;
    unsigned int msg_euid, msg_ruid, msg_suid, msg_fsuid;
    unsigned int msg_egid, msg_rgid, msg_sgid, msg_fsgid;
    char msg_evname[AUD_MAX_EVNAME];
  union {
    laus_login   loginData;
    laus_syscall syscallData;
    laus_exit    exitData;
    laus_netlink netlinkData;
    laus_text    textData;
  } laus_var_data;
} laus_data;

typedef int ARG_TYPE;
/*
typedef enum {
        AUDIT_ARG_IMMEDIATE = 1,
        AUDIT_ARG_POINTER,
        AUDIT_ARG_STRING,
        AUDIT_ARG_PATH,
        AUDIT_ARG_CWD,
        AUDIT_ARG_NULL,
        AUDIT_ARG_END = 0
} ARG_TYPE;
*/

#define AUDIT_ARG_IMMEDIATE_u 1234 
#define AUDIT_ARG_IGNORE 2345

#endif
