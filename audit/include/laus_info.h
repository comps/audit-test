#ifndef _LAUS_INFO_H
#define _LAUS_INFO_H

#include <limits.h>
#include <stdint.h>

/*
 * From SUSE <linux/audit.h>
 */

#define AUD_MAX_HOSTNAME	256
#define AUD_MAX_ADDRESS		256
#define AUD_MAX_TERMINAL	256
#define AUD_MAX_EVNAME		16

/*
 * Values for aud_msg_syscall.major and audit_policy.code
 */
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

struct aud_msg_syscall {
        int             personality;
        int             major, minor;
        int             result;
        unsigned int    length;
        unsigned char   data[0];
};

struct aud_msg_login {
        unsigned int    uid;
        char            hostname[AUD_MAX_HOSTNAME];
        char            address[AUD_MAX_ADDRESS];
        char            terminal[AUD_MAX_TERMINAL];
        char            executable[PATH_MAX];
};

struct aud_msg_exit {
	long            code;
};

struct aud_msg_netlink {
	unsigned int    groups, dst_groups;
	int             result;
	unsigned int    length;
	unsigned char   data[0];
};

/* Values for msg_type */
#define AUDIT_MSG_LOGIN		1
#define AUDIT_MSG_SYSCALL	2
#define AUDIT_MSG_EXIT		3
#define AUDIT_MSG_NETLINK	4
#define AUDIT_MSG_USERBASE	256     /* user land messages start here */

/*
 * From SUSE <audit.h>
 */
enum {
        AUDIT_MSG_TEXT = AUDIT_MSG_USERBASE,
};

/* Values for msg_arch */
enum {
        AUDIT_ARCH_I386,
        AUDIT_ARCH_PPC,
        AUDIT_ARCH_PPC64,
        AUDIT_ARCH_S390,
        AUDIT_ARCH_S390X,
        AUDIT_ARCH_X86_64,
        AUDIT_ARCH_IA64,
};

struct aud_message {
        uint32_t        msg_seqnr;
        uint16_t        msg_type;
        uint16_t        msg_arch;

        pid_t           msg_pid;
        size_t          msg_size;
        unsigned long   msg_timestamp;

        unsigned int    msg_audit_id;
        unsigned int    msg_login_uid;
        unsigned int    msg_euid, msg_ruid, msg_suid, msg_fsuid;
        unsigned int    msg_egid, msg_rgid, msg_sgid, msg_fsgid;

        /* Event name */
        char            msg_evname[AUD_MAX_EVNAME];

        unsigned char   msg_data[0];
};

/*
 * Encoding of arguments passed up to auditd
 */
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

#endif
