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

MACHINE		= $(strip $(shell uname -m))
X		= i486 i586 i686 ix86
P		= ppc powerpc
IP		= ppc64 powerpc64
Z		= s390
Z64		= s390x
X86_64		= x86_64
IA		= ia64
SYSTEMINFO      = systeminfo.run.log
CFLAGS          += -g -O2 -Wall -Werror -D_GNU_SOURCE
LDFLAGS         +=

LINK_AR		= $(AR) rc $@ $^
LINK_EXE	= $(CC) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)
LINK_SO		= $(CC) $(LDFLAGS) -shared -o $@ $^ $(LOADLIBES) $(LDLIBS)

# If MODE isn't set explicitly, the default for the machine is used
NATIVE          = $(strip $(shell file /bin/bash | awk -F'[ -]' '{print $$3}'))
MODE            ?= $(NATIVE)
ifneq ($(MODE), $(NATIVE))
    ifeq ($(MODE), 32)
	    ifneq (,$(findstring $(MACHINE), $(Z64)))
		    CFLAGS += -m31
		    LDFLAGS += -m31
	    else 
		    ifneq (,$(findstring $(MACHINE), $(X86_64)))
			    CFLAGS += -m32 -malign-double
			    LDFLAGS += -m32
		    else
			    CFLAGS += -m32
			    LDFLAGS += -m32
		    endif
	    endif
    endif
    ifeq ($(MODE), 64)
	    ifeq (,$(findstring $(MACHINE),$(X) $(IA)))
		    CFLAGS += -m64
		    LDFLAGS += -m64
	    endif
    endif
endif

##########################################################################
# Common rules
##########################################################################

.PHONY: all run \
	clean clobber distclean _clean _clobber _distclean \
	msgque rmlogs showrpms showrpms2

all: deps subdirs $(ALL_AR) $(ALL_EXE) $(ALL_SO)

run:

ifneq ($(if $(filter-out .,$(TOPDIR)),$(wildcard run.conf)),)
all: run.bash

run.bash:
	[[ -f run.bash ]] || ln -sfn $(TOPDIR)/utils/run.bash run.bash

run: all
	./run.bash
endif

_clean:
	@if [[ "$(MAKECMDGOALS)" == clean ]]; then \
	    for x in $(SUB_DIRS); do \
		make -C $$x clean; \
	    done; \
	fi
	$(RM) -r .deps
	$(RM) $(ALL_OBJ)
	$(RM) $(ALL_EXE) $(ALL_AR) $(ALL_SO)

clean: _clean

_distclean: clean
	@if [[ "$(MAKECMDGOALS)" == distclean ]]; then \
	    for x in $(SUB_DIRS); do \
		make -C $$x distclean; \
	    done; \
	fi
	$(RM) run.log
	if [[ -L run.bash ]]; then $(RM) run.bash; fi

distclean: _distclean

##########################################################################
# RPM dependency checking
##########################################################################

# These are assumed to be the base requirements for all the tests.  Requirements
# can be refined in individual Makefiles by appending (+=) or overriding (=)
# the RPMS variable.
RPMS		= binutils \
		  cpp \
                  expect \
                  flex \
                  gcc \
                  gcc-c++ \
                  glibc-devel \
                  libattr-devel \
                  libstdc++-devel \
                  make \
		  audit-libs-devel
ifneq ($(findstring $(MACHINE),$(IP)),)
RPMS		+= gcc-64bit 
endif

# This can be augmented per directory to check things other than the default
# list in "verify".  (In fact some things should be moved from that list to the
# appropriate directory)
verifyme: subdirs

verify:
	$(MAKE) verifyme
	@if ! mount | grep -q "^$$(df . | tail -n1 | cut -f1 -d\ ) .*(.*user_xattr"; then \
		echo "please set 'user_xattr' for this filesystem'"; \
		exit 1; \
	fi
	@if ! mount | grep -q "^$$(df . | tail -n1 | cut -f1 -d\ ) .*(.*acl"; then \
		echo "please set 'acl' for this filesystem'"; \
		exit 1; \
	fi
	@echo "-----------------------"
	@echo "Checking installed rpms"
	@echo "-----------------------"
	@if ! rpm -q $$($(MAKE) --no-print-directory showrpms); then \
	    echo "Please install the missing rpms"; \
	    exit 1; \
	fi
	@echo "-----------------------"
	@echo "Looks good!"

showrpms:
	@$(MAKE) --no-print-directory _showrpms | xargs -n1 echo | sort -u

_showrpms: subdirs_quiet
	@echo "$(RPMS)"

##########################################################################
# Dependency rules
##########################################################################

.PHONY: deps depsdir
DEP_FILES = $(addprefix .deps/, $(ALL_OBJS:.o=.d))

deps: depsdir $(DEP_FILES)

depsdir:
	@mkdir -p .deps

# See http://www.gnu.org/software/make/manual/html_node/make_47.html#SEC51
# "4.14 Generating Prerequisites Automatically"
.deps/%.d: %.c
	@echo Creating dependencies for $<
	@$(SHELL) -ec '$(CC) $(CFLAGS) $(INCLUDES) -MM $< \
		| sed '\''s@\($*\)\.o[ :]*@\1.o $@: @g'\'' > $@; \
		[ -s $@ ] || $(RM) $@'

-include .deps/*.d

# How to build missing things like libraries
../%:
	$(MAKE) -C $(dir $@) $(notdir $@)

##########################################################################
# Sub-directory processing rules
##########################################################################

.PHONY: subdirs subdirs_quiet

subdirs:
	@for x in $(SUB_DIRS); do \
	    $(MAKE) -C $$x $(MAKECMDGOALS); \
	done

subdirs_quiet:
	@for x in $(SUB_DIRS); do \
	    $(MAKE) --no-print-directory -C $$x $(MAKECMDGOALS); \
	done

##########################################################################
# Command framework execution rules
##########################################################################

systeminfo::
	echo "==> date <==" > $(SYSTEMINFO)
	date >> $(SYSTEMINFO)
	echo "" >> $(SYSTEMINFO)
	echo "==> uname -a <==" >> $(SYSTEMINFO)
	uname -a >> $(SYSTEMINFO)
	echo "" >> $(SYSTEMINFO)
	echo "==> uptime <==" >> $(SYSTEMINFO)
	uptime >> $(SYSTEMINFO)
	echo "" >> $(SYSTEMINFO)
	echo "==> cat /proc/cpuinfo <==" >> $(SYSTEMINFO)
	cat /proc/cpuinfo >> $(SYSTEMINFO)
	echo "" >> $(SYSTEMINFO)
	echo "==> rpm -qai <==" >> $(SYSTEMINFO)
	rpm -qai >> $(SYSTEMINFO)

ifneq ($(DEBUG),)
DEBUG_ARG	= -d $(DEBUG)
endif

ifneq ($(TEST_USER),)
USER_ARG	= -u $(TEST_USER)
endif

ifneq ($(LOGIN_USER),)
LOGIN_ARG	= -l $(LOGIN_USER)
endif

ifneq ($(TEST),)
TEST_ARG	= -t $(TEST)
endif
