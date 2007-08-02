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

AUDIT_TRAIL_EXE	= audit_trail_protection
AUDIT_TRAIL_OBJ	= audit_trail_protection.o

ALL_EXE = $(AUDIT_TRAIL_EXE)
ALL_OBJ = $(AUDIT_TRAIL_OBJ)

include $(TOPDIR)/rules.mk

$(AUDIT_TRAIL_EXE): $(AUDIT_TRAIL_OBJ)
	$(LINK_EXE)
