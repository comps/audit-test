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

FILTER_EXE	= filterconf
FILTER_OBJ	= filterconf.o
FILTER_AR	= $(TOPDIR)/utils/testutils.a	\
		  $(TESTDIR)/utils/filter_conf_utils.a	\
		  $(TESTDIR)/tests/filter_conf_tests.a

ALL_OBJ		= $(FILTER_OBJ)
ALL_EXE		= $(FILTER_EXE)
ALL_AR		= $(FILTER_AR)

SUB_DIRS	= utils tests

include $(TOPDIR)/rules.mk

$(FILTER_EXE): $(FILTER_OBJ) $(FILTER_AR)
	$(LINK_EXE)

run:
	$(AUDIT_CLEAN_LOG)
	./$(FILTER_EXE) $(DEBUG_ARG) $(USER_ARG) $(LOGIN_ARG) &> ./$(FILTER_EXE).run.log
