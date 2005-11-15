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
TESTDIR		= ..

UTILS_AR	= failsafe_utils.a
UTILS_OBJS	= failsafe_utils.o		\
		  failsafe_config.o

ALL_OBJS	= $(UTILS_OBJS)

INCLUDES	= -I$(TOPDIR)/include -I$(TESTDIR)/include

all: deps $(UTILS_AR)

$(UTILS_AR): $(UTILS_OBJS)
	$(AR) rc $@ $(UTILS_OBJS)

clean:
	$(RM) -r .deps
	$(RM) *.a *.o

include $(TOPDIR)/rules.mk
