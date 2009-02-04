###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

TOPDIR		= ../..
UTILSDIR	= ..
CPPFLAGS	+= -I$(UTILSDIR)/include
include $(TOPDIR)/rules.mk

CAPS_EXE	= do_capset

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

RT_EXE		= do_clock_settime

ONLY32_EXE	= do_chown32 \
		  do_fchown32 \
		  do_ioperm \
		  do_iopl \
		  do_lchown32 \
		  do_setfsgid32 \
		  do_setfsuid32 \
		  do_setgid32 \
		  do_setgroups32 \
		  do_setregid32 \
		  do_setresgid32 \
		  do_setresuid32 \
		  do_setreuid32 \
		  do_setuid32

ONLYIA64_EXE	= do_clone2

ONLY86_EXE	= do_fork \
		  do_vfork

ALL_EXE		= $(CAPS_EXE) \
		  $(CREATE_EXE) \
		  $(IPC_EXE) \
		  $(MQ_EXE) \
		  $(NET_EXE) \
		  $(RT_EXE) \
		  do_access \
		  do_acct \
		  do_adjtimex \
		  do_bind \
		  do_chdir \
		  do_chmod \
		  do_chown \
		  do_clone \
		  do_delete_module \
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
		  do_init_module \
		  do_ioctl \
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
		  do_setfsgid \
		  do_setfsuid \
		  do_setgid \
		  do_setgroups \
		  do_setregid \
		  do_setresgid \
		  do_setresuid \
		  do_setreuid \
		  do_settimeofday \
		  do_setuid \
		  do_setxattr \
		  do_socketcall \
		  do_stime \
		  do_swapon \
		  do_tgkill \
		  do_tkill \
		  do_truncate \
		  do_truncate64 \
		  do_umask \
		  do_unlink \
		  do_unlinkat \
		  do_uselib \
		  do_utime \
		  do_utimes

ifeq ($(MODE), 32)
ALL_EXE		+= $(ONLY32_EXE)
endif

ifeq ($(MACHINE), ia64)
ALL_EXE		+= $(ONLYIA64_EXE)
else
ALL_EXE		+= $(ONLY86_EXE)
endif

$(CAPS_EXE): LDLIBS += -lcap
$(CREATE_EXE): LDLIBS += -lselinux
$(MQ_EXE): LDLIBS += -lrt -lselinux
$(RT_EXE): LDLIBS += -lrt

all: $(ALL_EXE)

clobber:
	$(RM) $(ALL_EXE)
