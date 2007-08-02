###############################################################################
# Copyright (C) International Business Machines  Corp., 2003
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

TOPDIR		= ..
UTILSDIR	= .
CPPFLAGS 	+= -I$(UTILSDIR)/include
LDLIBS		+= -lselinux

UTILS_EXE	= test_context \
		  test_setcon

ALL_EXE		= $(UTILS_EXE)

SUB_DIRS	= bin network-server

include $(TOPDIR)/rules.mk

all: README.augrok chmod_utils

.PHONY: chmod_utils
chmod_utils:
	@chmod -R a+rX $$PWD

README.augrok: augrok
	pod2text augrok > $@
