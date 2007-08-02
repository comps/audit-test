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

RUN_DIRS	= syscalls \
		  network

ifeq ($(MODE), $(NATIVE))
RUN_DIRS       += fail-safe \
		  filter \
		  audit-trail-protection \
		  audit-tools \
		  libpam \
		  trustedprograms
endif

SUB_DIRS	= $(RUN_DIRS) \
		  utils

run: 
	@for l in run rollup; do \
	    echo "audit.$$l.log will be collected from subdirectories when the test completes" >audit.$$l.log; \
	done
	@$(check_set_PPROFILE); \
	$(check_set_PASSWD); \
	$(check_set_LBLNET_SVR_IPV4); \
	$(check_set_LBLNET_SVR_IPV6); \
	$(MAKE) all && \
	for x in $(RUN_DIRS); do make -C $$x run; done
	$(MAKE) report

.PHONY: dist
dist:
	deps=$$($(MAKE) --no-print-directory showrpms | xargs) && \
	rev=$$(svn info | awk '/^Revision:/{print $$2}' | grep .) && \
	tmpdir=$$(mktemp -d) && \
	into=$${PWD%/*} && \
	sed -i "/^Version:/s/[0-9].*/$$rev/; \
	        s/^\(Requires:[ 	]*\).*/\1$$deps/" \
		    audit-test.spec && \
	mkdir "$$tmpdir/audit-test" && \
	rsync -a --exclude=.svn . "$$tmpdir/audit-test" && \
	$(MAKE) distclean -C "$$tmpdir/audit-test" && \
	cd "$$tmpdir" && \
	chmod -R a+rX audit-test && \
	tar czf "$$into/audit-test-$$rev.tar.gz" audit-test && \
	cd "$$into" && \
	rm -rf "$$tmpdir" && \
	echo && \
	ls -l audit-test-$$rev.tar.gz

.PHONY: report
report:
	@# tty output has color, generated logs don't, so collect them
	@# instead of redirecting output of the above loop.
	@for l in run rollup; do \
	  ( utils/run.bash --header; echo ) &> audit.$$l.log; \
	  for x in $(RUN_DIRS); do \
	    echo; \
	    echo "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"; \
	    printf "::: %-52s :::\n" "$$x"; \
	    echo "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"; \
	    echo; \
	    cat $$x/$$l.log; \
	  done &> audit.$$l.log; \
	done
	utils/tally-results.awk < audit.rollup.log \
	    | tee -a audit.run.log | tee -a audit.rollup.log

.PHONY: rpm
rpm: dist
	rev=$$(svn info | awk '/^Revision:/{print $$2}' | grep .) && \
	cd .. && \
	rpmbuild -ta audit-test-$$rev.tar.gz && \
	mv /usr/src/redhat/RPMS/noarch/audit-test-$$rev-1.* . && \
	mv /usr/src/redhat/SRPMS/audit-test-$$rev-1.* . && \
	echo && \
	ls -l audit-test-$$rev-1.*

.PHONY: policy
policy:
	cd utils/selinux-policy && \
	$(MAKE) clean && \
	$(MAKE) && \
	$(MAKE) install && \
	$(MAKE) relabel

.PHONY: netconfig
netconfig:
	cd network/system && $(MAKE) install
	cd utils/network-server && $(MAKE) install
