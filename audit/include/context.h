/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
 * Written by Amy Griffis <amy.griffis@hp.com>
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   with this package; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * ======================================================================= 
 */

#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <linux/audit.h>
#include <limits.h>             /* for PATH_MAX */

/*
 * Context Flags
 */
#define NO_CHECK_SYSCALL_DATA   0x7ba970a7
#define NO_FORK			-3 
#define NO_ID_CHECK             -2
#define NO_PID_CHECK            -2
#define NO_RETURN_CODE          -2

/*
 * Syscall Args Encoding
 */
typedef int ARG_TYPE;
enum audit_arg {
        AUDIT_ARG_END,
        AUDIT_ARG_IMMEDIATE,
        AUDIT_ARG_POINTER,
        AUDIT_ARG_STRING,
        AUDIT_ARG_PATH,
        AUDIT_ARG_NULL,
        AUDIT_ARG_ERROR,
        AUDIT_ARG_VECTOR       /* for execve */
};
#define AUDIT_ARG_IMMEDIATE_u 1234 
#define AUDIT_ARG_IGNORE 2345

/*
 * Context Value Types
 */

/* message type */
#define AUDIT_MSG_LOGIN         0x001
#define AUDIT_MSG_SYSCALL	0x002
#define AUDIT_MSG_CWD           0x004
#define AUDIT_MSG_IPC           0x008
#define AUDIT_MSG_SOCKADDR      0x010
#define AUDIT_MSG_WATCH         0x020
#define AUDIT_MSG_PATH          0x040
#define AUDIT_MSG_USER          0x080
#define AUDIT_MSG_DAEMON        0x100

/* syscall arch */
#if defined(__i386__)
#define TS_BUILD_ARCH  AUDIT_ARCH_I386
#elif defined(__powerpc64__)
#define TS_BUILD_ARCH  AUDIT_ARCH_PPC64
#elif defined(__powerpc__)
#define TS_BUILD_ARCH  AUDIT_ARCH_PPC
#elif defined(__s390x__)
#define TS_BUILD_ARCH  AUDIT_ARCH_S390X
#elif defined(__s390__)
#define TS_BUILD_ARCH  AUDIT_ARCH_S390
#elif defined(__x86_64__)
#define TS_BUILD_ARCH  AUDIT_ARCH_X86_64
#elif defined(__ia64__)
#define TS_BUILD_ARCH  AUDIT_ARCH_IA64
#else
#define TS_BUILD_ARCH  -1
#endif

struct audit_syscall {
    char          *sysname;
    int           sysnum;
    int           arch;
    int           exit;
    /* ipc */
    unsigned int  ipc_qbytes;
    unsigned int  ipc_uid;
    unsigned int  ipc_gid;
    unsigned int  ipc_mode;
    /* filesystem */
    char          fs_cwd[PATH_MAX];
    char          fs_sobj[PATH_MAX];  /* source object */
    char          fs_tobj[PATH_MAX];  /* target object */
    char          fs_watch[PATH_MAX]; /* audit watch */
    /* will be deprecated */
    unsigned int  arglen;
    char          *args;
};

struct audit_user {
    unsigned int buflen;
    char         *buf;
};

struct audit_data {
    unsigned int type;
    unsigned int serial;
    time_t       timestamp;
    time_t       begin_time;
    time_t       end_time;
    pid_t        pid;
    uid_t        loginuid;
    uid_t        uid, euid, suid, fsuid;
    gid_t        gid, egid, sgid, fsgid;
    unsigned int success;
    int          error;
    int          experror;
    char         *comm;
    char         *exe;
    union {
        struct audit_syscall    syscall;
        struct audit_user       user;
    } u;
};

#endif	/* _CONTEXT_H */
