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

TOPDIR		= .

include $(TOPDIR)/rules.mk

#
# subdirs to run tests in
#

RUN_DIRS := \
	syscalls \
	syscalls-ns \
	syscalls-dac \
	network \
	misc

ifeq ($(MODE), $(NATIVE))
RUN_DIRS += \
	fail-safe \
	filter \
	audit-tools \
	libpam

ifneq ($(DISTRO), SUSE)
RUN_DIRS += \
	audit-remote \
	trustedprograms \
	crypto \
	netfilter \
	netfilebt \
	dbus \
	polkit

ifeq ($(MACHINE), x86_64)
ifeq ($(shell virt-what),)
RUN_DIRS += \
	kvm \
	kvm-cgroups \
	kvm-iommu
endif
endif
endif
endif

# customization logic to modify RUN_DIRS using external vars
ifneq ($(RUN_DIRS_SET),)
    RUN_DIRS := $(RUN_DIRS_SET)
endif
ifneq ($(RUN_DIRS_EXCLUDE),)
    RUN_DIRS := $(filter-out $(RUN_DIRS_EXCLUDE),$(RUN_DIRS))
endif
ifneq ($(RUN_DIRS_INCLUDE),)
    RUN_DIRS += $(RUN_DIRS_INCLUDE)
endif

SUB_DIRS := $(RUN_DIRS) utils

#
# common targets
#

run:
	@for l in run rollup; do \
	    echo "audit.$$l.log will be collected from subdirectories when the test completes" >audit.$$l.log; \
	done
	@$(check_set_PASSWD); \
	$(check_TTY); \
	$(MAKE) all && \
	for x in $(RUN_DIRS); do make -C $$x $(MAKECMDGOALS); done

rerun: run

.PHONY: report
ALL_LOGS	+= audit.run.log audit.rollup.log
report:
	@# tty output has color, generated logs don't, so collect them
	@# instead of redirecting output of the above loop.
	@for l in run rollup; do \
	  for x in $(RUN_DIRS); do \
	    echo; \
	    echo "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"; \
	    printf "::: %-52s :::\n" "$$x"; \
	    echo "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"; \
	    echo; \
	    cat $$x/$$l.log; \
	  done &> audit.$$l.log; \
	done
	utils/tally-results.sh < audit.rollup.log \
	    | tee -a audit.run.log | tee -a audit.rollup.log
	@utils/envcheck > envcheck.log

.PHONY: show-failed
show-failed:
	@for x in $(RUN_DIRS); do \
        [[ "$$x" =~ (network|netfilebt|netfilter) ]] && make -C $$x unlock &> /dev/null; \
        output=$$(cd $$x && ./run.bash -re --list); \
        [ "$$output" == "no matching tests" ] && continue; \
        [[ "$$output" =~ run.bash:\ error: ]] && continue; \
        printf "::: %s\n%s" "$$x" "$$output"; \
        echo; \
    done

.PHONY: show-rerun
show-rerun:
	@for x in $(RUN_DIRS); do \
		output=$$(cd $$x && ./run.bash -r --list); \
		[ "$$output" == "no matching tests" ] && continue; \
		[[ "$$output" =~ run.bash:\ error: ]] && continue; \
		printf "::: %s\n%s" "$$x" "$$output"; \
		echo; \
	done

.PHONY: rpm
rpm: dist
	rev=$$(git log | head -n 1| awk '/^commit/{print $$2}' | cut -b 1-6 ) && \
	cd .. && \
	rpmbuild -ta audit-test-$$rev.tar.gz && \
	mv ~/rpmbuild/RPMS/noarch/audit-test-$$rev-1.* . && \
	mv ~/rpmbuild/SRPMS/audit-test-$$rev-1.* . && \
	echo && \
	ls -l audit-test-$$rev-1.*

#
# suite rpm dependency listing
#

ifdef AUDIT_TEST_DEP
.PHONY: deplist
deplist:
	@echo '$(AUDIT_TEST_DEP)' | tr ' ' '\n'
.PHONY: depmissing
depmissing:
	@for pkg in $(AUDIT_TEST_DEP); do \
		rpm --quiet -q "$$pkg" || echo "$$pkg"; \
	done
endif

#
# user-friendly 'aliases' to utils/
#

.PHONY: policy
policy:
ifeq ($(PPROFILE),lspp)
	$(MAKE) -C utils/selinux-policy install
endif

.PHONY: ns_install
ns_install:
	$(MAKE) -C network/system install
	$(MAKE) -C utils/ns2 install

.PHONY: envcheck
envcheck:
	@$(check_set_PASSWD); \
	utils/envcheck

.PHONY: sssd-%
sssd-%:
	@utils/auth-server/sssd_setup $*

.PHONY: auth-server
auth-server: auth-server-install auth-server-configure auth-server-verify

.PHONY: auth-server-%
auth-server-%:
	@utils/auth-server/ipa_install $*
