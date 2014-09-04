##########################################################################
#   Copyright (C) International Business Machines  Corp., 2003
#   (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
#
#
#
#  FILE   : rules.mk
#
#  PURPOSE: This rules file facilitates the compiling, linking and running
#           of the Linux Auditing System test suite.
#
#           Rules are provided for dependency building, compiling, sub
#           directory traversal and running of the tests.
#
#
#  HISTORY:
#    08/03 originated by Tom Lendacky (toml@us.ibm.com)
#
##########################################################################

NATIVE		= $(strip $(shell file /bin/bash | awk -F'[ -]' '{print $$3}'))
MODE		?= $(NATIVE)
ifneq ($(MODE), $(NATIVE))
	ifeq ($(MODE), 32)
		ifneq (,$(findstring s390x, $(MACHINE)))
			CFLAGS += -m31
			LDFLAGS += -m31
		else
			CFLAGS += -m32
			LDFLAGS += -m32
		endif
	endif
	ifeq ($(MODE), 64)
		CFLAGS += -m64
		LDFLAGS += -m64
	endif
endif

export CFLAGS
export LDFLAGS
