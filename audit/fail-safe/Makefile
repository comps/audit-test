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

TOPDIR		= ../..
MAINDIR		= ..
CURRENTDIR	= .

CC		= gcc

AR		= ar
RANLIB		= ranlib

LN		= ln

FAILSAFE_EXE	= failsafe
FAILSAFE_OBJS	= failsafe.o
FAILSAFE_ARS	= $(MAINDIR)/utils/laus_test_utils.a	\
		  $(CURRENTDIR)/utils/failsafe_utils.a	\
		  $(CURRENTDIR)/tests/failsafe_tests.a

FAILSAFE_NOTIFY	= failsafe_notify

ALL_OBJS	= $(FAILSAFE_OBJS)

SUB_DIRS	= utils tests ../utils

INCLUDES	= -I$(MAINDIR)/include -I$(CURRENTDIR)/include
LIBS		+= -llaus

all:: subdirs deps $(FAILSAFE_EXE) $(FAILSAFE_NOTIFY)

$(FAILSAFE_EXE): $(FAILSAFE_OBJS) $(FAILSAFE_ARS)
	$(CC) -o $@ $(ARCH) $(FAILSAFE_OBJS) -Wl,--whole-archive -Wl,$(FAILSAFE_ARS) -Wl,--no-whole-archive $(LIBS)

$(FAILSAFE_NOTIFY): $(FAILSAFE_EXE)
	$(LN) -sf $< $@

run:: all
	./$(FAILSAFE_EXE) $(DEBUG_ARG) $(USER_ARG) $(LOGIN_ARG) &> ./$(FAILSAFE_EXE).run.log

clean:: subdirs
	$(RM) *.a *.o $(FAILSAFE_EXE) $(FAILSAFE_NOTIFY)

include $(TOPDIR)/rules.mk
