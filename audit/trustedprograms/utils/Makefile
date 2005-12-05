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
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TESTDIR)/include

TP_UTILS_AR	= trustedprogram_test_utils.a
TP_UTILS_OBJ	= post_trustedprogram.o		\
		  pre_trustedprogram.o		\
		  trusted_program.o		\
		  shadow_test_setup.o

ALL_AR		= $(TP_UTILS_AR)
ALL_OBJ		= $(TP_UTILS_OBJ)

include $(TOPDIR)/rules.mk

$(TP_UTILS_AR): $(TP_UTILS_OBJ)
	$(LINK_AR)
