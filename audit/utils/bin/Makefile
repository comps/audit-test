###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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
###############################################################################

TOPDIR		= ../..
CPPFLAGS	+= -I$(TOPDIR)/include

CREATE_EXE	= do_creat\
		  do_mkdir \
		  do_mkdirat \
		  do_mknod \
		  do_mknodat \
		  do_open \
		  do_openat \
		  do_symlink \
		  do_symlinkat

IPC_EXE		= do_ipc \
		  do_msgctl \
		  do_msgget \
		  do_msgrcv \
		  do_msgsnd \
		  do_semctl \
		  do_semget \
		  do_semop \
		  do_semtimedop \
		  do_shmat \
		  do_shmctl \
		  do_shmget

MQ_EXE		= do_mq_open \
		  do_mq_unlink

NET_EXE		= do_accept \
		  do_connect \
		  do_read \
		  do_recvfrom \
		  do_recvmsg \
		  do_sendmsg \
		  do_sendto

ONLY32_EXE	= do_chown32 \
		  do_fchown32 \
		  do_lchown32

ALL_EXE		= $(CREATE_EXE) \
		  $(IPC_EXE) \
		  $(MQ_EXE) \
		  $(NET_EXE) \
		  do_access \
		  do_acct \
		  do_adjtimex \
		  do_bind \
		  do_chdir \
		  do_chmod \
		  do_chown \
		  do_clock_settime \
		  do_dummy \
		  do_dummy_group \
		  do_execve \
		  do_fchmod \
		  do_fchmodat \
		  do_fchown \
		  do_fchownat \
		  do_fremovexattr \
		  do_fsetxattr \
		  do_getseconds \
		  do_gettimezone \
		  do_getxattr \
		  do_kill \
		  do_lchown \
		  do_lgetxattr \
		  do_link \
		  do_linkat \
		  do_listxattr \
		  do_llistxattr \
		  do_loop \
		  do_loop_clear \
		  do_lremovexattr \
		  do_lsetxattr \
		  do_mount \
		  do_ptrace \
		  do_readlink \
		  do_readlinkat \
		  do_removexattr \
		  do_rename \
		  do_renameat \
		  do_rmdir \
		  do_settimeofday \
		  do_setxattr \
		  do_socketcall \
		  do_stime \
		  do_swapon \
		  do_tgkill \
		  do_tkill \
		  do_truncate \
		  do_truncate64 \
		  do_unlink \
		  do_unlinkat \
		  do_uselib \
		  do_utime \
		  do_utimes

ifeq ($(MODE), 32)
ALL_EXE		+= $(ONLY32_EXE)
endif

include $(TOPDIR)/rules.mk

$(CREATE_EXE): LDLIBS += -lselinux
$(MQ_EXE): LDLIBS += -lrt -lselinux

all: $(ALL_EXE)

clobber:
	$(RM) $(ALL_EXE)
