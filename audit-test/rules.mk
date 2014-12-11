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

# defined by parent
export TOPDIR

#
# initial definitions
#

SHELL		:= /bin/bash
CFLAGS		+= -g -O2 -Wall -Werror -D_GNU_SOURCE -fno-strict-aliasing

#
# MACHINE - architecture of this machine
#

MACHINE		:= $(strip $(shell uname -m))

# architecture aliases
archalias = $(if $(findstring $(1),$(2)),$(3),$(1))
#      		                               from                     to
MACHINE		:= $(call archalias,$(MACHINE),i386 i486 i586 i686 ix86,i686)
MACHINE		:= $(call archalias,$(MACHINE),powerpc,ppc)
MACHINE		:= $(call archalias,$(MACHINE),powerpc64,ppc64)

export MACHINE

# MACHINE32 - 32bit multilib of MACHINE
ifeq ($(MACHINE),x86_64)
    export MACHINE32 := i686
else ifeq ($(MACHINE),ppc64)
    export MACHINE32 := ppc
else ifeq ($(MACHINE),s390x)
    export MACHINE32 := s390
endif

#
# MODE - bitness of this machine (may be overriden by user)
#

NATIVE		:= $(strip $(shell file /bin/bash | awk -F'[ -]' '{print $$3}'))
MODE		?= $(NATIVE)
ifneq ($(MODE), $(NATIVE))
    ifeq ($(MODE),32)
        ifeq ($(MACHINE),s390x)
            CFLAGS += -m31
            LDFLAGS += -m31
        else
            CFLAGS += -m32
            LDFLAGS += -m32
        endif
    else ifeq ($(MODE),64)
        CFLAGS += -m64
        LDFLAGS += -m64
    endif
endif

export MODE

#
# DISTRO - current OS distribution
#

# crude detection
DISTRO := $(wildcard /etc/*-release)

ifneq (,$(findstring SuSE,$(DISTRO)))
    DISTRO := SUSE
else ifneq (,$(findstring fedora,$(DISTRO)))
    DISTRO := FEDORA
else ifneq (,$(findstring redhat,$(DISTRO)))
    DISTRO := RHEL
endif

export DISTRO

# selinux
ifneq (,$(findstring $(DISTRO),RHEL FEDORA))
    export LSM_SELINUX := 1
endif

#
# additional CFLAGS exports for conditional binary compilation
#

ifdef MACHINE
    CFLAGS += -DMACHINE_$(MACHINE)
endif
ifdef DISTRO
    CFLAGS += -DDISTRO_$(DISTRO)
endif
ifdef LSM_SELINUX
    CFLAGS += -DLSM_SELINUX
endif

#
# AUDIT_TEST_DEP, suite (rpm) dependencies
#

# ie. pkgname.noarch everywhere
AUDIT_TEST_DEP_NOARCH := \
	selinux-policy-devel \
	perl-Expect

# ie. pkgname.x86_64 on x86_64 or pkgname.s390x on s390x
AUDIT_TEST_DEP_NATIVE := \
	tar \
	bzip2 \
	binutils \
	cpp \
	flex \
	make \
	gcc \
	gcc-c++ \
	libstdc++ \
	libstdc++-devel \
	perl-devel \
	perl-IO-Tty \
	krb5-workstation \
	\
	expect \
	strace \
	kpartx \
	setools-console \
	iptables \
	ebtables \
	patch \
	nmap-ncat \
	psmisc \
	xinetd

# ie. pkgname.i686 on x86_64 or pkgname.s390 on s390x
AUDIT_TEST_DEP_MULTILIB :=

# both native and multilib versions (ie. pkgname.x86_64 and pkgname.i686)
AUDIT_TEST_DEP_BOTH := \
	glibc \
	glibc-devel \
	libgcc \
	libattr \
	libattr-devel \
	libcap \
	libcap-devel \
	audit-libs \
	audit-libs-devel \
	libselinux \
	libselinux-devel \
	keyutils-libs \
	keyutils-libs-devel \
	xfsprogs-devel \
	dbus-devel

# special cases
# libseccomp only on x86,
# kvm virt stuff only on x86 (for now)
ifneq (,$(findstring $(MACHINE),x86_64 i686))
    AUDIT_TEST_DEP_NATIVE += libseccomp libseccomp-devel
    AUDIT_TEST_DEP_MULTILIB += libseccomp libseccomp-devel
    AUDIT_TEST_DEP_NATIVE += libvirt qemu-kvm virt-install
endif

# add pkg name suffixes
AUDIT_TEST_DEP_NATIVE += $(AUDIT_TEST_DEP_BOTH)
AUDIT_TEST_DEP := \
	$(addsuffix .noarch,$(AUDIT_TEST_DEP_NOARCH)) \
	$(addsuffix .$(MACHINE),$(AUDIT_TEST_DEP_NATIVE))
ifdef MACHINE32
    AUDIT_TEST_DEP_MULTILIB += $(AUDIT_TEST_DEP_BOTH)
    AUDIT_TEST_DEP += $(addsuffix .$(MACHINE32),$(AUDIT_TEST_DEP_MULTILIB))
endif

export AUDIT_TEST_DEP

#
# syscall relevancy
#
# - needs to be here as it may be needed in any bucket,
#   even when running via ./run.bash
#
# note that SCREL_FILE may be overwritten by make(1) args

ifdef DISTRO
    SCREL_FILE := $(wildcard $(TOPDIR)/utils/bin/relevancy-$(DISTRO))
endif
ifeq (,$(SCREL_FILE))
    # if DISTRO is unset or relevancy-$DISTRO doesn't exist
    SCREL_FILE := $(TOPDIR)/utils/bin/relevancy  # fallback
endif

# the -x check is required, see utils/Makefile
SCREL_SYSCALLS := $(shell [ -x "$(TOPDIR)/utils/screl-parser.py" ] && \
                          "$(TOPDIR)/utils/screl-parser.py" \
                          $(SCREL_FILE) $(MACHINE) $(MODE))

export SCREL_SYSCALLS

##########################################################################
# Common rules
##########################################################################

.PHONY: all run \
	clean distclean verify _clean _distclean _verify

all: deps subdirs $(ALL_AR) $(ALL_EXE) $(ALL_SO)

run:

rerun:

# Re-used in toplevel Makefile
check_set_PPROFILE = \
	if [[ ! -x /usr/sbin/getenforce ]]; then \
	  export PPROFILE=capp ; \
        elif [[ $$PPROFILE != capp && $$PPROFILE != lspp ]]; then \
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

ifeq (, $(findstring network, $(RUN_DIRS)))
check_set_LBLNET_SVR_IPV4 = true
else
check_set_LBLNET_SVR_IPV4 = \
	while [[ -z $$LBLNET_SVR_IPV4 ]]; do \
	    trap 'stty echo; exit' 1 2; \
	    read -p "Remote test server IPv4 address: " LBLNET_SVR_IPV4; \
		echo; export LBLNET_SVR_IPV4; \
	    trap - 1 2; \
	done
endif

ifeq (, $(findstring network, $(RUN_DIRS)))
check_set_LBLNET_SVR_IPV6 = true
else
check_set_LBLNET_SVR_IPV6 = \
	while [[ -z $$LBLNET_SVR_IPV6 ]]; do \
	    trap 'stty echo; exit' 1 2; \
	    read -p "Remote test server IPv6 address: " LBLNET_SVR_IPV6; \
		echo; export LBLNET_SVR_IPV6; \
	    trap - 1 2; \
	done
endif

check_TTY = \
	if [[ -f /etc/selinux/mls/contexts/securetty_types ]]; then \
	    tty=`/usr/bin/tty`; \
	    tty_type=`ls -lZ $$tty | awk -F: '{print $$3}' | awk '{print $$1}'`; \
	    grep -q $$tty_type /etc/selinux/mls/contexts/securetty_types /dev/null && { \
	        echo -n "You are connected to the test machine through "; \
	        echo "a device ($$tty) that"; \
	        echo -n "will prevent one or more tests from functioning "; \
	        echo "as intended.  Connect to"; \
	        echo -n "the machine remotely through a pty device, such "; \
	        echo "as logging in as the "; \
	        echo "test-user directly using ssh."; \
	        echo ; \
	        exit 1; \
	    } \
	fi

ifneq ($(if $(filter-out .,$(TOPDIR)),$(wildcard run.conf)),)
all: run.bash

run.bash:
	[[ -f run.bash ]] || ln -sfn $(TOPDIR)/utils/run.bash run.bash

run: all
	@$(check_set_PPROFILE); \
	$(check_set_PASSWD); \
	./run.bash --header; \
	./run.bash

rerun: all
	@$(check_set_PPROFILE); \
	$(check_set_PASSWD); \
	./run.bash --rerun
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

ALL_LOGS += run.log rollup.log logs
_distclean: clean
	@if [[ "$(MAKECMDGOALS)" == distclean ]]; then \
	    for x in $(SUB_DIRS); do \
		make -C $$x distclean; \
	    done; \
	fi
	$(RM) -r $(ALL_LOGS)
	if [[ -L run.bash ]]; then $(RM) run.bash; fi

distclean: _distclean

_verify:

verify: _verify

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
# Various helper rules
##########################################################################

# needs SHELL set to bash (due to printf %q)
export_env:
	@IFS=$$'\n'; for var in $$(env); do \
		printf 'export %q\n' "$$var"; \
	done;
