#########################################################################
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
#           files in the current directory that end in ".c" (source code).
#
#           The "clean" target removes all files the end in ".o" (object
#           files).
#
#
#  HISTORY:
#    08/03 originated by Daniel H. Jones <danjones@us.ibm.com>
#    08/03 modified by Dustin Kirkalnd (k1rkland@us.ibm.com)
#
##########################################################################

TOPDIR		= ../..
TESTDIR		= ..
CC		= gcc
INCLUDES	= -I$(TOPDIR)/include -I$(TESTDIR)/include

AR		= ar
RANLIB		= ranlib

UTILS_AR	= libpam_test_utils.a
UTILS_OBJS	= post_pam_program.o		\
		  pre_pam_program.o		\
		  pam_program.o


all: $(UTILS_AR)

$(UTILS_AR): $(UTILS_OBJS)
	$(AR) rc $@ $(UTILS_OBJS)
	$(RANLIB) $@

clean:
	$(RM) *.a *.o

include $(TOPDIR)/rules.mk
