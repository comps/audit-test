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
SUB_DIRS        = fail-safe \
		  syscalls 
#		  audbin \
#		  audit_tools \
#		  audit_trail_protection \
#		  filter-conf \
#		  libpam \
#		  pam_laus \
#		  trustedprograms

all: subdirs

run:
	$(MAKE) all
	for x in $(SUB_DIRS); do make -C $$x run; done 2>&1 | ./tally-results.awk

.PHONY: rpm
rpm:
	rev=$$(svn info | awk '/^Revision:/{print $$2}'); \
	    sed -i "/^Version:/s/[0-9].*/$$rev/" audit-testsuite.spec; \
	    mkdir /tmp/audit-testsuite-$$rev || exit; \
	    rsync -a --exclude=.svn . /tmp/audit-testsuite-$$rev; \
	    $(MAKE) clobber -C /tmp/audit-testsuite-$$rev; \
	    cd /tmp; \
	    tar czf audit-testsuite-$$rev.tar.gz audit-testsuite-$$rev; \
	    rpmbuild -ta audit-testsuite-$$rev.tar.gz; \
	    rm -rf /tmp/audit-testsuite-$$rev

include $(TOPDIR)/rules.mk
