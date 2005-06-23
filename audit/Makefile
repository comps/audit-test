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
		  audbin \
		  audit_tools \
		  audit_trail_protection \
		  filter-conf \
		  libpam \
		  pam_laus \
		  trustedprograms \
		  syscalls 
LOG		= run.log

all: subdirs

clean:: subdirs

msgque:
	./rm_msgque

run: msgque subdirs
	make report	

report:
	tail -n 1 */*.run.log > $(LOG)
	echo "" >> $(LOG)
	echo "----------------------------------" >> $(LOG)
	echo "" >> $(LOG)
	echo "FAIL:" >> $(LOG)
	-grep -H "\WFAIL\W" */*.run.log >> $(LOG)
	-grep -H "\Wfailed\W" audit_trail_protection/*.run.log >> $(LOG)
	echo "ERROR:" >> $(LOG)
	-grep -H "\WERROR\W" */*.run.log >> $(LOG)

cleanup::
	grep "^laus_" /etc/passwd | awk -F: '{print $$1}' | xargs -i userdel -r {}
	grep "^laus_" /etc/group | awk -F: '{print $$1}' | xargs -i groupdel {}
	rm -rf /tmp/laus??????

include $(TOPDIR)/rules.mk
