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
		  do_mknod \
		  do_open \
		  do_symlink

IPC_EXE		= do_ipc \
		  do_msgget \
		  do_msgrcv \
		  do_msgsnd \
		  do_semget \
		  do_semop \
		  do_semtimedop \
		  do_shmat \
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

ONLY32_EXE	= do_fchown32

ALL_EXE		= $(CREATE_EXE) \
		  $(IPC_EXE) \
		  $(MQ_EXE) \
		  $(NET_EXE) \
		  do_access \
		  do_dummy \
		  do_dummy_group \
		  do_execve \
		  do_fchmod \
		  do_fchmodat \
		  do_fchown \
		  do_fchownat \
		  do_fremovexattr \
		  do_fsetxattr \
		  do_getxattr \
		  do_kill \
		  do_lgetxattr \
		  do_link \
		  do_linkat \
		  do_listxattr \
		  do_llistxattr \
		  do_lremovexattr \
		  do_lsetxattr \
		  do_mkdirat \
		  do_mknodat \
		  do_mount \
		  do_openat \
		  do_ptrace \
		  do_readlink \
		  do_readlinkat \
		  do_removexattr \
		  do_rename \
		  do_renameat \
		  do_rmdir \
		  do_setxattr \
		  do_symlinkat \
		  do_tgkill \
		  do_tkill \
		  do_truncate \
		  do_truncate64 \
		  do_unlink \
		  do_unlinkat \
		  do_uselib

ifeq ($(MODE), 32)
ALL_EXE		+= $(ONLY32_EXE)
endif

include $(TOPDIR)/rules.mk

$(CREATE_EXE): LDLIBS += -lselinux
$(MQ_EXE): LDLIBS += -lrt
$(ALL_EXE): LDLIBS += -lselinux

all: $(ALL_EXE)

clobber:
	$(RM) $(ALL_EXE)
