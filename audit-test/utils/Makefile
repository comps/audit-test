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

include $(TOPDIR)/rules.mk

CPPFLAGS 	+= -I$(UTILSDIR)/include
ifdef LSM_SELINUX
LDLIBS		+= -lselinux

ALL_EXE		= test_context test_setcon
endif

SUB_DIRS	= bin namespaces
ifdef LSM_SELINUX
SUB_DIRS	+= ns2
endif

executables: $(ALL_EXE)

# not built by default
README.augrok: augrok
	pod2text augrok > $@

.PHONY: README.augrok_clean
distclean: README.augrok_clean
README.augrok_clean:
	$(RM) README.augrok
