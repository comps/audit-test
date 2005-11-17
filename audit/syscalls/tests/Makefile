##########################################################################
#   Copyright (C) International Business Machines  Corp., 2003
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#
#
#  FILE   : Makefile
#
#  PURPOSE: The default "all" target uses the gcc compiler to compile all
#           test files in the current directory.
#
#           The "clean" target removes all files the end in ".o" (object
#           files).
#
#
#  HISTORY:
#    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com
#    06/03 furthered by Michael A. Halcrow <mhalcrow@cs.byu.edu>
#    08/03 reimplemented by Daniel H. Jones <danjones@us.ibm.com>
#
##########################################################################

TOPDIR		= ../..
TESTDIR		= ..
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TESTDIR)/include

TESTS_AR	= syscall_test.a
TESTS_OBJ	= test_access.o \
		  test_adjtimex.o \
		  test_bind.o \
		  test_capset.o \
		  test_chdir.o \
		  test_chmod.o \
		  test_chown.o \
		  test_chown16.o \
		  test_chown32.o \
		  test_clone.o \
		  test_clone2.o \
		  test_creat.o \
		  test_delete_module.o \
		  test_execve.o \
		  test_fchmod.o \
		  test_fchown.o \
		  test_fchown16.o \
		  test_fchown32.o \
		  test_fork.o \
		  test_fremovexattr.o \
		  test_fsetxattr.o \
		  test_init_module.o \
		  test_ioctl.o \
		  test_ioperm.o \
		  test_iopl.o \
		  test_lchown.o \
		  test_lchown16.o \
		  test_lchown32.o \
		  test_link.o \
		  test_lremovexattr.o \
		  test_lsetxattr.o \
		  test_mkdir.o \
		  test_mknod.o \
		  test_mount.o \
		  test_msgctl.o \
		  test_msgget.o \
		  test_msgsnd.o \
		  test_msgrcv.o \
		  test_open.o \
		  test_ptrace.o \
		  test_removexattr.o \
		  test_rename.o \
		  test_rmdir.o \
		  test_semctl.o \
		  test_semget.o \
		  test_semop.o \
		  test_semtimedop.o \
		  test_setfsgid.o \
		  test_setfsuid.o \
		  test_setgid.o \
		  test_setgroups.o \
		  test_setgroups16.o \
		  test_setgroups32.o \
		  test_setregid.o \
		  test_setresgid.o \
		  test_setresuid.o \
		  test_setreuid.o \
		  test_settimeofday.o \
		  test_setuid.o \
		  test_setxattr.o \
		  test_shmat.o \
		  test_shmctl.o \
		  test_shmget.o \
		  test_swapon.o \
		  test_symlink.o \
		  test_truncate.o \
		  test_truncate64.o \
		  test_umask.o \
		  test_unlink.o \
		  test_utime.o \
		  test_utimes.o \
		  test_vfork.o \

ALL_OBJ		= $(TESTS_OBJ)
ALL_AR		= $(TESTS_AR)

all: $(ALL_AR)

$(TESTS_AR): $(TESTS_OBJ)
	$(LINK_AR)

include $(TOPDIR)/rules.mk
