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
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TESTDIR)/include

SYSCALLS_OBJ	= syscalls.o syscalls_table.o
SYSCALLS_EXE	= syscalls

UTILS_AR = $(TOPDIR)/utils/testutils.a
TESTS_AR = tests/syscall_test.a

ALL_OBJ = $(SYSCALLS_OBJ)
ALL_EXE = $(SYSCALLS_EXE)

SUB_DIRS = kmods tests utils ../utils

all: subdirs deps $(EXE)

run: all
	./run.bash

$(SYSCALLS_EXE): LDLIBS += -laudit
$(SYSCALLS_EXE): $(ALL_OBJ) $(UTILS_AR) $(TESTS_AR)
	$(LINK_EXE)

include $(TOPDIR)/rules.mk
