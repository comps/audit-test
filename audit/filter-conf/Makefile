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
#  PURPOSE: The default "all" target will build all targets within this
#           makefile.
#
#           The "clean" target removes all targets and all intermediate
#           files.
#
#
#  HISTORY:
#    08/03 originated by Tom Lendacky (toml@us.ibm.com)
#
##########################################################################

TOPDIR		= ..
CURRENTDIR	= .

CC		= gcc

AR		= ar
RANLIB		= ranlib

FILTER_EXE	= filterconf
FILTER_OBJS	= filterconf.o
FILTER_ARS	= $(TOPDIR)/utils/laus_test_utils.a	\
		  $(CURRENTDIR)/utils/filter_conf_utils.a	\
		  $(CURRENTDIR)/tests/filter_conf_tests.a

ALL_OBJS	= $(FILTER_OBJS)

SUB_DIRS	= utils tests ../utils

INCLUDES	= -I$(TOPDIR)/include -I$(CURRENTDIR)/include
LIBS		+= -llaus

all:: subdirs deps $(FILTER_EXE)

$(FILTER_EXE): $(FILTER_OBJS) $(FILTER_ARS)
	$(CC) -o $@ $(FILTER_OBJS) $(ARCH) -Wl,--whole-archive -Wl,$(FILTER_ARS) -Wl,--no-whole-archive $(LIBS)

run::
	$(AUDIT_CLEAN_LOG)
	./$(FILTER_EXE) $(DEBUG_ARG) $(USER_ARG) $(LOGIN_ARG) &> ./$(FILTER_EXE).run.log

clean:: subdirs
	$(RM) *.a *.o $(FILTER_EXE)

include $(TOPDIR)/rules.mk
