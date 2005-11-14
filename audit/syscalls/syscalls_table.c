/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
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

#include "includes.h"
#include "syscalls.h"

/*
 * Syscall Test Lookup Table
 */
struct syscall_tests {
    int         (*testp)(struct audit_data *);
    char        *testname;
};

struct syscall_tests syscall_table[] = {
    { &test_access, "access" }, 
    { &test_adjtimex, "adjtimex" }, 
    { &test_bind, "bind" }, 
    { &test_capset, "capset" }, 
    { &test_chdir, "chdir" }, 
    { &test_chmod, "chmod" }, 
    { &test_chown, "chown" }, 
    { &test_chown16, "chown16" }, 
    { &test_chown32, "chown32" }, 
    { &test_chroot, "chroot" }, 
    { &test_clone, "clone" }, 
    { &test_clone2, "clone2" }, 
    { &test_creat, "creat" }, 
    { &test_delete_module, "delete_module" }, 
    { &test_execve, "execve" }, 
    { &test_fchdir, "fchdir" }, 
    { &test_fchmod, "fchmod" }, 
    { &test_fchown, "fchown" }, 
    { &test_fchown16, "fchown16" }, 
    { &test_fchown32, "fchown32" }, 
    { &test_fork, "fork" }, 
    { &test_fremovexattr, "fremovexattr" }, 
    { &test_fsetxattr, "fsetxattr" }, 
    { &test_ftruncate, "ftruncate" }, 
    { &test_ftruncate64, "ftruncate64" }, 
    { &test_init_module, "init_module" }, 
    { &test_ioctl, "ioctl" },
    { &test_ioperm, "ioperm" }, 
    { &test_iopl, "iopl" }, 
    { &test_kill, "kill" }, 
    { &test_lchown, "lchown" }, 
    { &test_lchown16, "lchown16" }, 
    { &test_lchown32, "lchown32" }, 
    { &test_link, "link" }, 
    { &test_lremovexattr, "lremovexattr" }, 
    { &test_lsetxattr, "lsetxattr" }, 
    { &test_mkdir, "mkdir" }, 
    { &test_mknod, "mknod" }, 
    { &test_mount, "mount" }, 
    { &test_msgctl, "msgctl" }, 
    { &test_msgsnd, "msgsnd" },
    { &test_msgrcv, "msgrcv" },
    { &test_msgget, "msgget" }, 
    { &test_open,  "open" },
    { &test_ptrace,  "ptrace" }, 
    { &test_reboot, "reboot" }, 
    { &test_removexattr, "removexattr" }, 
    { &test_rename, "rename" }, 
    { &test_rmdir, "rmdir" }, 
    { &test_semctl, "semctl" }, 
    { &test_semget, "semget" }, 
    { &test_semop, "semop" },
    { &test_semtimedop, "semtimedop" },
    { &test_setdomainname, "setdomainname" }, 
    { &test_setfsgid, "setfsgid" }, 
    { &test_setfsgid16, "setfsgid16" }, 
    { &test_setfsgid32, "setfsgid32" }, 
    { &test_setfsuid, "setfsuid" }, 
    { &test_setfsuid16, "setfsuid16" }, 
    { &test_setfsuid32, "setfsuid32" }, 
    { &test_setgid, "setgid" }, 
    { &test_setgid16, "setgid16" }, 
    { &test_setgid32, "setgid32" }, 
    { &test_setgroups, "setgroups" }, 
    { &test_setgroups16, "setgroups16" }, 
    { &test_setgroups32, "setgroups32" }, 
    { &test_sethostname, "sethostname" }, 
    { &test_setpgid, "setpgid" }, 
    { &test_setpriority, "setpriority" }, 
    { &test_setregid, "setregid" }, 
    { &test_setregid16, "setregid16" },
    { &test_setregid32, "setregid32" }, 
    { &test_setresgid, "setresgid" }, 
    { &test_setresgid16, "setresgid16" }, 
    { &test_setresgid32, "setresgid32" }, 
    { &test_setresuid, "setresuid" }, 
    { &test_setresuid16, "setresuid16" }, 
    { &test_setresuid32, "setresuid32" }, 
    { &test_setreuid, "setreuid" }, 
    { &test_setreuid16, "setreuid16" }, 
    { &test_setreuid32, "setreuid32" }, 
    { &test_setrlimit, "setrlimit" }, 
    { &test_setsid, "setsid" }, 
    { &test_settimeofday, "settimeofday" }, 
    { &test_setuid, "setuid" }, 
    { &test_setuid16, "setuid16" }, 
    { &test_setuid32, "setuid32" }, 
    { &test_setxattr, "setxattr" }, 
    { &test_shmat, "shmat" }, 
    { &test_shmctl, "shmctl" }, 
    { &test_shmdt, "shmdt" }, 
    { &test_shmget, "shmget" }, 
    { &test_swapoff, "swapoff" }, 
    { &test_swapon, "swapon" }, 
    { &test_symlink, "symlink" }, 
    { &test_syslog, "syslog" },  
    { &test_tkill, "tkill" },
    { &test_truncate, "truncate" }, 
    { &test_truncate64, "truncate64" }, 
    { &test_umask, "umask" }, 
    { &test_unlink, "unlink" }, 
    { &test_utime, "utime" }, 
    { &test_utimes, "utimes" }, 
    { &test_vfork, "vfork" }, 
};

int lookup_testcase(int (**test_h)(), char *testcase)
{
    int i;

    for (i = 0; i < sizeof(syscall_table)/sizeof(syscall_table[0]); i++)
	if (!strcmp(testcase, syscall_table[i].testname)) {
	    *test_h = syscall_table[i].testp;
	    return 0;
	}
    return -1;
}
