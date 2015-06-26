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

#
# required libraries
#

# for SELinux context file operation
ifdef LSM_SELINUX
CREATE_EXE	= do_creat \
		  do_mkdir \
		  do_mkdirat \
		  do_mknod \
		  do_mknodat \
		  do_open \
		  do_openat \
		  do_symlink \
		  do_symlinkat \

$(CREATE_EXE): LDLIBS += -lselinux
endif

# POSIX message queues need librt (and SELinux since they work with files)
MQ_EXE		= do_mq_open \
		  do_mq_unlink
$(MQ_EXE): LDLIBS += -lrt
ifdef LSM_SELINUX
$(MQ_EXE): LDLIBS += -lselinux
endif

# additional specific library rules
do_clock_settime: LDLIBS += -lrt
do_capset: LDLIBS += -lcap

#
# syscall inclusion according to relevancy
#

$(call parse_screl)
ALL_EXE	:= $(addprefix do_,$(SCREL_SYSCALLS))

executables: $(ALL_EXE)
