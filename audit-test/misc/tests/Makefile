###############################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
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

include $(TOPDIR)/rules.mk

EXECUTABLE=do_tty devfileperm dirperm fileperm namedpipes_fifoperm suid_sgid unixdomainsocketperm semperm shmperm msqperm \
	sys_procperms \
	objreuse-brk objreuse-ftruncate objreuse-lseek objreuse-mmap objreuse-msg objreuse-sem objreuse-shm \
	checkaccess sparse_file cgroup_limits cgroup_exec

# seccomp only on x86
ifneq (,$(findstring $(MACHINE),x86_64 i686))
    EXECUTABLE += seccomp
endif

all: $(EXECUTABLE)

namedpipes_fifoperm: LDLIBS += -pthread
seccomp: LDLIBS += -lseccomp

clean:
	rm -f $(EXECUTABLE) extract_dir/* *.log
