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

#if defined(__MODE_32) || defined(__IX86)
typedef long long __laus_int64;
typedef int __laus_int32;
#else
typedef long __laus_int64;
typedef int __laus_int32;
#endif

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

/* msg_type */
#define AUDIT_MSG_LOGIN		1
#define AUDIT_MSG_SYSCALL	2
#define AUDIT_MSG_EXIT		3
#define AUDIT_MSG_NETLINK	4
#define AUDIT_MSG_TEXT          256
#define AUDIT_MSG_USERBASE	256     /* user land messages start here */

/* msg_arch */
enum {
        AUDIT_ARCH_I386,
        AUDIT_ARCH_PPC,
        AUDIT_ARCH_PPC64,
        AUDIT_ARCH_S390,
        AUDIT_ARCH_S390X,
        AUDIT_ARCH_X86_64,
        AUDIT_ARCH_IA64,
};
#if defined(__IX86)
#define AUDIT_ARCH  AUDIT_ARCH_I386
#elif defined(__PPC32)
#define AUDIT_ARCH  AUDIT_ARCH_PPC
#elif defined(__PPC64)
#define AUDIT_ARCH  AUDIT_ARCH_PPC64
#elif defined(__S390X)
#define AUDIT_ARCH  AUDIT_ARCH_S390X
#elif defined(__S390)
#define AUDIT_ARCH  AUDIT_ARCH_S390
#elif defined(__X86_64)
#define AUDIT_ARCH  AUDIT_ARCH_X86_64
#elif defined(__IA64)
#define AUDIT_ARCH  AUDIT_ARCH_IA64
#else
#define AUDIT_ARCH  -1
#endif

/* syscall numbers */
#define AUDIT_invalid		-1
#define AUDIT_access 		1049
#define AUDIT_acct		1064
#define AUDIT_adjtimex		1131
#define AUDIT_brk		1060
#define AUDIT_bind		1191
#define AUDIT_capset		1186
#define AUDIT_chdir		1034
#define AUDIT_chmod		1038
#define AUDIT_chown		1039
#define AUDIT_chroot		1068
#define AUDIT_clone		1128
#define AUDIT_clone2		1213
#define AUDIT_delete_module	1134
#define AUDIT_execve		1033
#define AUDIT_fchdir		1035
#define AUDIT_fchmod		1099
#define AUDIT_fchown		1100
#define AUDIT_fremovexattr 	1228
#define AUDIT_fsetxattr		1219
#define AUDIT_ftruncate		1098
#define AUDIT_init_module	1133
#define AUDIT_ioctl		1065
#define AUDIT_ioperm		0	// XXX
#define AUDIT_iopl		0	// XXX
#define AUDIT_kill		1053
#define AUDIT_lchmod		0	// XXX
#define AUDIT_lchown		1124
#define AUDIT_link		1031
#define AUDIT_lremovexattr	1227
#define AUDIT_lsetxattr		1218
#define AUDIT_mkdir		1055
#define AUDIT_mknod		1037
#define AUDIT_mount		1043
#define AUDIT_msgctl		1112
#define AUDIT_msgget		1109
#define AUDIT_msgrcv		1111
#define AUDIT_msgsnd		1110
#define AUDIT_open		1028
#define AUDIT_ptrace		1048
#define AUDIT_quotactl		1137
#define AUDIT_reboot		1096
#define AUDIT_removexattr	1226
#define AUDIT_rename		1054
#define AUDIT_rmdir		1056
#define AUDIT_semctl		1108
#define AUDIT_semget		1106
#define AUDIT_semop		1107
#define AUDIT_semtimedop	1247
#define AUDIT_setdomainname	1129
#define AUDIT_setfsgid		1143
#define AUDIT_setfsuid		1142
#define AUDIT_setgid		1061
#define AUDIT_setgroups		1078
#define AUDIT_sethostname	1083
#define AUDIT_setpgid		1080
#define AUDIT_setpriority	1102
#define AUDIT_setregid		1072
#define AUDIT_setresgid		1076
#define AUDIT_setresuid		1074
#define AUDIT_setreuid		1071
#define AUDIT_setrlimit		1084
#define AUDIT_setsid		1081
#define AUDIT_settimeofday	1088
#define AUDIT_setuid		1045
#define AUDIT_setxattr		1217
#define AUDIT_shmat		1114
#define AUDIT_shmctl		1116
#define AUDIT_shmdt		1115
#define AUDIT_shmget		1113
#define AUDIT_socket		1190
#define AUDIT_swapoff		1095
#define AUDIT_swapon		1094
#define AUDIT_symlink		1091
#define AUDIT_syslog		1117
#define AUDIT_tgkill		1235
#define AUDIT_tkill		1229
#define AUDIT_truncate		1097
#define AUDIT_umask		1067
#define AUDIT_umount		1044
#define AUDIT_unlink		1032
#define AUDIT_utimes		1036
#define audit_NUM_CALLS 81

/*
 * This set of data structures collects system context to compare with
 * a resulting audit record.  Some members may be initialized prior to
 * the operation, while others are populated with the operation
 * results.
 */
#define AUD_MAX_ADDRESS		256
#define AUD_MAX_EVNAME		16
#define AUD_MAX_HOSTNAME	256
#define AUD_MAX_TERMINAL	256

typedef struct {
    uid_t         uid;
    char          hostname[AUD_MAX_HOSTNAME];
    char          address[AUD_MAX_ADDRESS];
    char          terminal[AUD_MAX_TERMINAL];
    char	  executable[PATH_MAX];
} laus_login ;

typedef struct {
    int           personality;
    int		  code;
    int		  minor;
    int		  result;
    int           resultErrno;
    unsigned int  length;
    unsigned char *data;	/* variable size */
} laus_syscall;

typedef struct {
    long code;
} laus_exit;

typedef struct {
    unsigned int  groups;
    unsigned int  dst_groups;
    int           result;
    unsigned int  length;
    unsigned char *data;	/* variable size */
} laus_netlink;

typedef struct {
    unsigned char *data;	/* variable size */
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
    char         msg_evname[AUD_MAX_EVNAME];
    union {
        laus_login   loginData;
        laus_syscall syscallData;
        laus_exit    exitData;
        laus_netlink netlinkData;
        laus_text    textData;
    } laus_var_data;
} laus_data;

#endif	/* _CONTEXT_H */
