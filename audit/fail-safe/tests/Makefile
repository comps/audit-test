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

TOPDIR		= ../..
TESTDIR		= ..

FPRINTF_OBJ	= fprintf.o
FPRINTF_SO	= fprintf.so

ALL_OBJ		= $(FPRINTF_OBJ)
ALL_SO		= $(FPRINTF_SO)

ifeq ($(MODE),)
$(FPRINTF_SO): CFLAGS += -fPIC
$(FPRINTF_SO): $(FPRINTF_OBJ)
	$(LINK_SO)
else
# fprintf.so is preloaded for auditd, so must be built for the native
# architecture.  See test_disk_error_action.bash
$(FPRINTF_SO):
	$(MAKE) "$@" MODE=
endif

include $(TOPDIR)/rules.mk
