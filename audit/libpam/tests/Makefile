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

TOPDIR		= ../..
TESTDIR		= ..
CPPFLAGS        += -I$(TOPDIR)/include -I$(TESTDIR)/include

TESTS_AR	= libpam_test.a
TESTS_OBJ	= test_sshd.o \
		  test_su.o \
		  test_vsftpd.o \
		  test_login.o

ALL_AR		= $(TESTS_AR)
ALL_OBJ		= $(TESTS_OBJ)

include $(TOPDIR)/rules.mk

$(TESTS_AR): $(TESTS_OBJ)
	$(LINK_AR)
