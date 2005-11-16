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
#	   files in the current directory that end in ".c" (source code).
#
#	   The "clean" target removes all files the end in ".o" (object
#	   files).
#
#
#  HISTORY:
#    08/03 originated by Dustin Kirkland (k1rkland@us.ibm.com
#
##########################################################################

TOPDIR		= ../..
TESTDIR		= ..
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TESTDIR)/include

TESTS_AR	= pam_laus_test.a
TESTS_OBJ	= test_sshd.o \
		  test_vsftpd.o 

ALL_AR		= $(TESTS_AR)
ALL_OBJ		= $(TESTS_OBJ)

all: $(TESTS_AR)

$(TESTS_AR): $(OBJ)
	$(LINK_AR)

include $(TOPDIR)/rules.mk
