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
#    08/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
#
##########################################################################

MAKE=make
CC=gcc
INCLUDES=-I./include -I../include
EXE=pam_laus
TOPDIR          = ..

UTILS_AR = $(TOPDIR)/utils/laus_test_utils.a

PAM_LAUS_OBJS   = pam_laus.o

ALL_OBJS        = $(PAM_LAUS_OBJS)

SUB_DIRS        = tests utils ../utils

all: subdirs deps $(EXE)

$(EXE): $(ALL_OBJS) $(UTILS_AR)
	$(CC) $(FLAGS) $(CFLAGS) $(ARCH) -o $(EXE) pam_laus.o ./tests/*.o ./utils/*.o -Wl,--whole-archive -Wl,$(UTILS_AR) -Wl,--no-whole-archive 

run: all
	$(AUDIT_CLEAN_LOG)
	./$(EXE) &> ./$(EXE).run.log

clean: 
	make clean -C tests
	make clean -C utils
	$(RM) -r .deps
	$(RM) *.o

clobber: clean
	$(RM) $(EXE)

include $(TOPDIR)/rules.mk
