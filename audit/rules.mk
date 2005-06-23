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

MACHINE		:= $(strip $(shell uname -m))
X		:= i486 i586 i686 ix86
P		:= ppc powerpc
IP		:= ppc64 powerpc64
Z		:= s390
Z64		:= s390x
X86_64		:= x86_64
HOME_DIR	:= $(shell pwd | awk -F"laus_test" '{print $$1}')
SYSTEMINFO      := systeminfo.run.log
FLAGS           := -g -O2 -Wall -D_GNU_SOURCE

AUDIT_CLEAN_LOG := /etc/init.d/audit stop; /bin/rm -f /var/log/audit.d/*; /etc/init.d/audit start

DEPS		:=  binutils \
		    cpp \
                    expect \
                    flex \
                    gcc \
                    gcc-c++ \
                    glibc-devel \
                    kernel-source \
                    laus-devel \
                    libattr-devel \
                    libstdc++-devel \
                    make \
		    perl-Expect \
		    perl-IO-Tty \
		    perl-IO-Stty\
                    tcl
PPC64_DEPS	:=  gcc-64bit 
WARN_DEPS	:=  

ifneq (,$(findstring $(MACHINE),$(X)))
	ARCH := -D__IX86
else
	ifneq (,$(findstring $(MACHINE),$(P)))
		ARCH := -D__PPC32 -D__PPC
	else
		ifneq (,$(findstring $(MACHINE),$(IP)))
			ARCH := -D__PPC64 -D__PPC
			DEPS := $(DEPS) $(PPC64_DEPS)
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
	ifeq (,$(findstring $(MACHINE),$(X)))
		ARCH += -m64 -D__MODE_64
		export LIB_DIR = /lib64
		export LDFLAGS = -m64
	endif
endif


.PHONY: all clean deps depsdir subdirs $(SUB_DIRS) test run \
	cleanup extract msgque report rmlogs

#
# Compile rules
#
%.o: %.c Makefile
	$(CC) $(FLAGS) $(ARCH) $(INCLUDES) -c -o $@ $<

#
# Dependency rules
#
DEP_FILES	= $(addprefix .deps/, $(ALL_OBJS:.o=.d))

deps::  headers depsdir $(DEP_FILES)

depsdir::
	@mkdir -p .deps

headers:: 

ifeq ($(findstring clean,$(MAKECMDGOALS)),)
# Include dependencies if goals do not include 'clean'
.deps/%.d: %.c
	@echo Creating dependencies for $<
	@$(SHELL) -ec '$(CC) $(FLAGS) $(CFLAGS) $(INCLUDES) -MM $< \
		| sed '\''s@\($*\)\.o[ :]*@\1.o $@: @g'\'' > $@; \
		[ -s $@ ] || $(RM) $@'

-include .deps/*.d
endif

#
# Sub-directory processing rules
#
subdirs: $(SUB_DIRS)

$(SUB_DIRS):
	$(MAKE) $(COMPILER) -C $@ $(MAKECMDGOALS)

#
# Command framework execution rules
#
test:: subdirs

#run:: cleanup verifydeps all

rmlogs:: subdirs
	-find . | grep "run.log" | xargs -i rm -f {}

verifydeps::
	# MODE must be defined to either MODE=32 or MODE=64
	@if test -z "$$PASSWD"; \
	then \
		echo "ERROR: You must export PASSWD for LTP tests!!"; \
		exit 1; \
	fi
	@if echo $$PATH | grep '\.' >/dev/null; \
	then :; else \
		echo "ERROR: You must put '.' in PATH for LTP tests!!"; \
		exit 1; \
	fi
	@if echo $$PATH | grep '/usr/sbin' >/dev/null; \
	then :; else \
		echo "ERROR: You must put /sbin and /usr/sbin in PATH for LTP tests!!"; \
		exit 1; \
	fi
	@if sed 's/#.*//' /etc/audit/audit.conf | grep 'sync.*=.*no' >/dev/null; \
	then :; else \
		echo "ERROR: put 'sync = no;' in /etc/audit/audit.conf !!"; \
		exit 1; \
	fi
	@if sed 's/#.*//' /etc/audit/audit.conf | grep 'audbin.*-S' >/dev/null; \
	then \
		echo "ERROR: remove '-S <filename>' from notify entry in /etc/audit/audit.conf !!"; \
		exit 1; \
	fi
	@if mount | grep '^'`df . | awk '/dev/{print $$1}'` | grep '(.*user_xattr' >/dev/null; \
	then :; else \
		echo "ERROR: set 'user_xattr' option for this file system!!"; \
		exit 1; \
	fi
	@if mount | grep '^'`df . | awk '/dev/{print $$1}'` | grep '(.*acl' >/dev/null; \
	then :; else \
		echo "ERROR: set 'acl' option for this file system!!"; \
		exit 1; \
	fi
	@rpm -q $(DEPS) >/dev/null
	@find /usr/lib/perl5/ -type f -name "Expect.pm" | grep "Expect.pm" >/dev/null
#	-@rpm -q $(WARN_DEPS) >/dev/null

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
