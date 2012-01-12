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

SHELL := /bin/bash

MACHINE		:= $(strip $(shell uname -m))
X86		:= i486 i586 i686 ix86
PPC		:= ppc powerpc
PPC64		:= ppc64 powerpc64
Z		:= s390
Z64		:= s390x
X86_64		:= x86_64
IA64		:= ia64

export NATIVE   = $(strip $(shell file /bin/bash | awk -F'[ -]' '{print $$3}'))
export MODE     ?= $(NATIVE)

ifneq (,$(findstring $(MACHINE),$(X86)))
	ARCH := -D__IX86
else
	ifneq (,$(findstring $(MACHINE),$(PPC)))
		ARCH := -D__PPC32 -D__PPC
	else
		ifneq (,$(findstring $(MACHINE),$(PPC64)))
			ARCH := -D__PPC64 -D__PPC
		else
			ifneq (,$(findstring $(MACHINE),$(Z)))
				ARCH := -D__S390
			else
				ifneq (,$(findstring $(MACHINE), $(Z64)))
					ARCH := -D__S390X
				else
					ifneq (,$(findstring $(MACHINE), $(X86_64)))
						ARCH := -D__X86_64
					endif
				endif
			endif
		endif
	endif
endif
ifeq ($(MODE), 32)
	ifneq (,$(findstring $(MACHINE), $(Z64)))
		ARCH += -m31 -D__MODE_32
		export LDFLAGS = -m31
	else
		ifneq (,$(findstring $(MACHINE), $(X86_64)))
			ARCH += -m32 -D__MODE_32 -malign-double
			export LDFLAGS = -m32
		else
			ARCH += -m32 -D__MODE_32
			export LDFLAGS = -m32
		endif
	endif
else
        ifeq (,$(findstring $(MACHINE),$(X86)))
                ARCH += -D__MODE_64
        else
		ifeq (,$(findstring $(MACHINE),$(X86)))
			ARCH += -m64 -D__MODE_64
			export LIB_DIR = /lib64
			export LDFLAGS = -m64
		endif
	endif
endif

export CFLAGS += $(ARCH)
