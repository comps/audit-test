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
#  FILE   : Makefile
#
#  PURPOSE: This makefile facilitates the compiling and linking of the
#           Linux Auditing System test suite.
#
#  HISTORY:
#    09/03 originated by Kylene Smith (kylene@us.ibm.com)
#
##########################################################################

TOPDIR		= .
SUB_DIRS        = utils \
		  fail-safe \
		  syscalls \
		  audit_trail_protection \
#		  audit_tools \
#		  filter-conf \
#		  libpam \
#		  trustedprograms

include $(TOPDIR)/rules.mk

run:
	$(MAKE) all
	for x in $(SUB_DIRS); do make -C $$x run; done 2>&1 | utils/tally-results.awk
	@# tty output has color, generated run.logs don't, so collect them
	@# instead of redirecting output of the above loop.
	@for x in $(SUB_DIRS); do \
	    echo; \
	    echo "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"; \
	    printf "::: %-52s :::\n" "$$x"; \
	    echo "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"; \
	    echo; \
	    cat $$x/run.log; \
	done 2>&1 | utils/tally-results.awk > run.log

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
	tar czf "$$into/audit-test-$$rev.tar.gz" audit-test && \
	cd "$$into" && \
	rm -rf "$$tmpdir" && \
	echo && \
	ls -l audit-test-$$rev.tar.gz

.PHONY: rpm
rpm: dist
	rev=$$(svn info | awk '/^Revision:/{print $$2}' | grep .) && \
	cd .. && \
	rpmbuild -ta audit-test-$$rev.tar.gz && \
	mv /usr/src/redhat/RPMS/noarch/audit-test-$$rev-1.* . && \
	mv /usr/src/redhat/SRPMS/audit-test-$$rev-1.* . && \
	echo && \
	ls -l audit-test-$$rev-1.*
