###############################################################################
# Copyright (C) International Business Machines  Corp., 2003
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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

TOPDIR		= ..
TESTDIR		= .
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TESTDIR)/include

SYSCALLS_OBJ	= syscalls.o syscalls_table.o
SYSCALLS_EXE	= syscalls

UTILS_AR	= $(TOPDIR)/utils/testutils.a
TUTILS_AR	= utils/syscall_test_utils.a
TESTS_AR	= tests/syscall_test.a

ALL_OBJ		= $(SYSCALLS_OBJ)
ALL_EXE		= $(SYSCALLS_EXE)

SUB_DIRS	= tests utils

include $(TOPDIR)/rules.mk

$(SYSCALLS_EXE): LDLIBS += -laudit
$(SYSCALLS_EXE): $(ALL_OBJ) $(TESTS_AR) $(UTILS_AR) $(TUTILS_AR)
	$(LINK_EXE)
