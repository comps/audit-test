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
    char        *testname;
    int         (*testp)(struct audit_data *, int, int);
};

struct syscall_variations {
    char        *varname;
    int		varnum;
};

static struct syscall_tests syscall_table[] = {
    { "access", &test_access }, 
    { "adjtimex", &test_adjtimex }, 
    { "bind", &test_bind }, 
    { "capset", &test_capset }, 
    { "chdir", &test_chdir }, 
    { "chmod", &test_chmod }, 
    { "chown32", &test_chown32 }, 
    { "chown", &test_chown }, 
    { "clock_settime", &test_clock_settime }, 
    { "clone2", &test_clone2 }, 
    { "clone", &test_clone }, 
    { "creat", &test_creat }, 
    { "delete_module", &test_delete_module }, 
    { "execve", &test_execve }, 
    { "fchmod", &test_fchmod }, 
    { "fchown32", &test_fchown32 }, 
    { "fchown", &test_fchown }, 
    { "fork", &test_fork }, 
    { "fremovexattr", &test_fremovexattr }, 
    { "fsetxattr", &test_fsetxattr }, 
    { "init_module", &test_init_module }, 
    { "ioctl", &test_ioctl },
    { "ioperm", &test_ioperm }, 
    { "iopl", &test_iopl }, 
    { "lchown32", &test_lchown32 }, 
    { "lchown", &test_lchown }, 
    { "link", &test_link }, 
    { "lremovexattr", &test_lremovexattr }, 
    { "lsetxattr", &test_lsetxattr }, 
    { "mkdir", &test_mkdir }, 
    { "mknod", &test_mknod }, 
    { "mount", &test_mount }, 
    { "msgctl", &test_msgctl }, 
    { "msgget", &test_msgget }, 
    { "msgrcv", &test_msgrcv },
    { "msgsnd", &test_msgsnd },
    { "open", &test_open },
    { "ptrace", &test_ptrace }, 
    { "removexattr", &test_removexattr }, 
    { "rename", &test_rename }, 
    { "rmdir", &test_rmdir }, 
    { "semctl", &test_semctl }, 
    { "semget", &test_semget }, 
    { "semop", &test_semop },
    { "semtimedop", &test_semtimedop },
    { "setfsgid32", &test_setfsgid32 }, 
    { "setfsgid", &test_setfsgid }, 
    { "setfsuid32", &test_setfsuid32 }, 
    { "setfsuid", &test_setfsuid }, 
    { "setgid32", &test_setgid32 }, 
    { "setgid", &test_setgid }, 
    { "setgroups32", &test_setgroups32 }, 
    { "setgroups", &test_setgroups }, 
    { "setregid32", &test_setregid32 }, 
    { "setregid", &test_setregid }, 
    { "setresgid32", &test_setresgid32 }, 
    { "setresgid", &test_setresgid }, 
    { "setresuid32", &test_setresuid32 }, 
    { "setresuid", &test_setresuid }, 
    { "setreuid32", &test_setreuid32 }, 
    { "setreuid", &test_setreuid }, 
    { "settimeofday", &test_settimeofday }, 
    { "setuid32", &test_setuid32 }, 
    { "setuid", &test_setuid }, 
    { "setxattr", &test_setxattr }, 
    { "shmat", &test_shmat }, 
    { "shmctl", &test_shmctl }, 
    { "shmget", &test_shmget }, 
    { "stime", &test_stime }, 
    { "swapon", &test_swapon }, 
    { "symlink", &test_symlink }, 
    { "truncate64", &test_truncate64 }, 
    { "truncate", &test_truncate }, 
    { "umask", &test_umask }, 
    { "unlink", &test_unlink }, 
    { "utimes", &test_utimes }, 
    { "utime", &test_utime }, 
    { "vfork", &test_vfork }, 
};

static struct syscall_variations syscall_vtable[] = {
    { "basic", TESTSC_BASIC },
    { "remove", TESTSC_REMOVE },
    { "setperms", TESTSC_SETPERMS },
    { "file", TESTSC_FILE },
    { "symlink", TESTSC_SYMLINK },
    { "modify", TESTSC_MODIFY },
    { "nomodify", TESTSC_NOMODIFY },
};

int (*lookup_testcase(char *testcase))(struct audit_data *, int, int)
{
    int i;

    for (i = 0; i < sizeof(syscall_table)/sizeof(syscall_table[0]); i++)
	if (!strcmp(testcase, syscall_table[i].testname)) {
	    return syscall_table[i].testp;
	}
    return NULL;
}

int lookup_variation(char *varstr)
{
    int i;

    for (i = 0; i < sizeof(syscall_vtable)/sizeof(syscall_vtable[0]); i++)
	if (!strcmp(varstr, syscall_vtable[i].varname)) {
	    return syscall_vtable[i].varnum;
	}
    return -1;
}
