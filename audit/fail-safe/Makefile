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
#   along with this pronram;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#
#
#  FILE   : Makefile
#
#  PURPOSE: The default "all" target will build all targets within this
#           makefile.
#
#           The "clean" target removes all targets and all intermediate
#           files.
#
#
#  HISTORY:
#    09/03 originated by Tom Lendacky (toml@us.ibm.com)
#
##########################################################################

TOPDIR		= ..
TESTDIR		= .
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TESTDIR)/include

FAILSAFE_EXE	= failsafe
FAILSAFE_OBJ	= failsafe.o
FAILSAFE_AR	= $(TOPDIR)/utils/testutils.a	\
		  $(TESTDIR)/utils/failsafe_utils.a	\
		  $(TESTDIR)/tests/failsafe_tests.a

ALL_OBJ		= $(FAILSAFE_OBJ)
ALL_AR		= $(FAILSAFE_AR)
ALL_EXE		= $(FAILSAFE_EXE) $(FAILSAFE_NOTIFY)

FAILSAFE_NOTIFY	= failsafe_notify
FAILSAFE_LOGINUID	= test_pam_loginuid.sh

SUB_DIRS	= utils tests $(TOPDIR)/utils

LDLIBS		+= -llaus

all: subdirs deps $(ALL_EXE)

$(FAILSAFE_EXE): $(FAILSAFE_OBJS) $(FAILSAFE_ARS)
	$(LINK_EXE)

.PHONY: $(FAILSAFE_NOTIFY)
$(FAILSAFE_NOTIFY): $(FAILSAFE_EXE)
	ln -sfn $< $@

#run: all
#	./$(FAILSAFE_EXE) $(DEBUG_ARG) $(USER_ARG) $(LOGIN_ARG) &> ./$(FAILSAFE_EXE).run.log
run:
	cd tests; $(FAILSAFE_LOGINUID) 2>&1 >> ../$(FAILSAFE_EXE).run.log

include $(TOPDIR)/rules.mk
