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
#  PURPOSE: This makefile facilitates the compiling and linking of the
#           Linux Auditing System test suite.
#            
#           The default "all" target will compile all files ending in ".c"
#           in the current directory, after executing "make" in the "./tests"
#           subdirectory and the "./utils" subdirectory.  It will then link
#           the ".o" object files created and produce an executable "syscalls".
#
#           The "clean" target will remove all files ending in ".o" and the
#           "syscalls" binary from the current directory, as well as
#           executing "make clean" in the "./tests" and the "./utils" 
#           subdirectories.
#
#
#  HISTORY:
#    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
#    06/03 Updated by Michael A. Halcrow <mike@halcrow.us>
#    08/03 Updated by Daniel H. Jones <danjones@us.ibm.com>
#
##########################################################################
TOPDIR		= ..
TESTDIR		= .
MAKE		= make
CC		= gcc
INCLUDES	= -I$(TOPDIR)/include -I$(TESTDIR)/include
EXE		= syscalls

UTILS_AR = $(TOPDIR)/utils/testutils.a
TESTS_AR = tests/syscall_test.a

SYSCALL_OBJS = syscalls.o syscalls_table.o

ALL_OBJS = $(SYSCALL_OBJS)

SUB_DIRS = kmods tests utils ../utils

LIBS += -laudit

all: subdirs deps $(EXE)

run: all
ifeq ($(shell whoami),root)
#	$(AUDIT_CLEAN_LOG)
	./run.bash
else
	@echo "You must run this as root"
endif

$(EXE): $(ALL_OBJS) $(UTILS_AR) $(TESTS_AR)
	$(CC) $(CFLAGS) -o $(EXE) $(ALL_OBJS) tests/*.o utils/*.o -Wl,--whole-archive -Wl,$(UTILS_AR) -Wl,--no-whole-archive $(LIBS)

clean:
	make clean -C kmods
	make clean -C tests
	make clean -C utils
	$(RM) -r .deps
	$(RM) *.o

clobber: clean
	$(RM) $(EXE)

include $(TOPDIR)/rules.mk
