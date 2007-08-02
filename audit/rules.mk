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
export NATIVE   = $(strip $(shell file /bin/bash | awk -F'[ -]' '{print $$3}'))
export MODE     ?= $(NATIVE)
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

# Re-used in toplevel Makefile
check_set_PPROFILE = \
	if [[ $$PPROFILE != capp && $$PPROFILE != lspp ]]; then \
	  export PPROFILE=capp ; \
	  if [[ "$$(getenforce)" == "Enforcing" ]] &&  \
	        (/usr/sbin/sestatus | grep -q mls); then \
	    if [[ "$$(secon -r)" != "lspp_test_r" ]]; then \
	      echo "SELinux MLS policy is enabled but you are not in lspp_test_r" ; \
	      exit 1; \
	    else \
	      export PPROFILE=lspp ; \
	    fi \
	  fi \
	fi 

check_set_PASSWD = \
	while [[ -z $$PASSWD ]]; do \
	    trap 'stty echo; exit' 1 2; \
	    read -sp "Login user password: " PASSWD; echo; export PASSWD; \
	    trap - 1 2; \
	done 

check_set_LBLNET_SVR_IPV4 = \
	while [[ -z $$LBLNET_SVR_IPV4 ]]; do \
	    trap 'stty echo; exit' 1 2; \
	    read -p "Remote test server IPv4 address: " LBLNET_SVR_IPV4; \
		echo; export LBLNET_SVR_IPV4; \
	    trap - 1 2; \
	done

check_set_LBLNET_SVR_IPV6 = \
	while [[ -z $$LBLNET_SVR_IPV6 ]]; do \
	    trap 'stty echo; exit' 1 2; \
	    read -p "Remote test server IPv6 address: " LBLNET_SVR_IPV6; \
		echo; export LBLNET_SVR_IPV6; \
	    trap - 1 2; \
	done

ifneq ($(if $(filter-out .,$(TOPDIR)),$(wildcard run.conf)),)
all: run.bash

run.bash:
	[[ -f run.bash ]] || ln -sfn $(TOPDIR)/utils/run.bash run.bash

run: all
	@$(check_set_PPROFILE); \
	$(check_set_PASSWD); \
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
	@if ! mount | grep -q "^$$(df . | head -n2 | tail -n1 | cut -f1 -d\ ) .*(.*user_xattr"; then \
		echo "please set 'user_xattr' for this filesystem'"; \
		exit 1; \
	fi
	@if ! mount | grep -q "^$$(df . | head -n2 | tail -n1 | cut -f1 -d\ ) .*(.*acl"; then \
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

DEP_FILES = $(addprefix .deps/, $(ALL_OBJ:.o=.d))

.PHONY: deps

deps: $(DEP_FILES)

# See http://www.gnu.org/software/make/manual/html_node/make_47.html#SEC51
# "4.14 Generating Prerequisites Automatically"
.deps/%.d: %.c
	@mkdir -p .deps
	@echo Creating dependencies for $<
	@$(SHELL) -ec '$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< \
		| sed '\''s@\($*\)\.o[ :]*@\1.o $@: @g'\'' > $@; \
		[ -s $@ ] || $(RM) $@'

ifneq ($(DEP_FILES),)
-include $(DEP_FILES)
endif

# How to build missing things like libraries
../%:
	$(MAKE) -C $(dir $@) $(notdir $@)

##########################################################################
# Sub-directory processing rules
##########################################################################

.PHONY: subdirs subdirs_quiet

subdirs:
	@for x in $(SUB_DIRS); do \
	    $(MAKE) -C $$x $(MAKECMDGOALS) || exit $$?; \
	done

subdirs_quiet:
	@for x in $(SUB_DIRS); do \
	    $(MAKE) --no-print-directory -C $$x $(MAKECMDGOALS) || exit $$?; \
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
