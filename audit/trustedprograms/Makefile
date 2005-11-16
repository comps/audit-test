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
#           the ".o" object files created and produce an executable "laustest".
#
#           The "clean" target will remove all files ending in ".o" and the
#           "laustest" binary from the current directory, as well as
#           executing "make clean" in the "./tests" and the "./utils" 
#           subdirectories.
#
#
#  HISTORY:
#    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com
#    06/03 Updated by Michael A. Halcrow <mhalcrow@cs.byu.edu>
#    08/03 Updated by Daniel H. Jones <danjones@us.ibm.com>
#
##########################################################################

TOPDIR		= ..
TESTDIR		= .
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TESTDIR)/include

TP_EXE		= trustedprograms
TP_OBJ		= trustedprograms.o

UTILS_AR	= $(TOPDIR)/utils/testutils.a

ALL_OBJ		= $(TP_OBJ)
ALL_EXE		= $(TP_EXE)

SUB_DIRS	= tests utils ../utils

all: deps $(ALL_EXE)

$(TP_EXE): LDLIBS += -llaus
$(TP_EXE): $(TP_OBJS) $(UTILS_AR)
	$(CC) $(LDFLAGS) -o $@ $^ tests/*.o utils/*.o $(LOADLIBES) $(LDLIBS)

run: all
	$(AUDIT_CLEAN_LOG)
	./$(EXE) &> ./$(EXE).run.log

include $(TOPDIR)/rules.mk
