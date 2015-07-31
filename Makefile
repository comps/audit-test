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
#           entire test suite.
#
#  HISTORY:
#    11/03 originated by Dustin Kirkland (k1rkland@us.ibm.com)
#    08/04 updated by Dustin Kirkland (k1rkland@us.ibm.com)
#    04/07 updated by Lisa Smith (lisa.m.smith@hp.com)
#    11/11 updated by Ondrej Moris (omoris@redhat.com)
#
##########################################################################

TOPDIR		= .

SUB_DIRS	= audit-test ltp

REPORT		= logs-*.tar.gz
SYSTEMINFO	= systeminfo.run.log
SUMMARY		= run.log
EXTRA_DIST_FILES = Makefile README

# generic recursion into all SUB_DIRS
.PHONY: subdirs
subdirs:
	@for x in $(SUB_DIRS); do \
		$(MAKE) -C $$x $(MAKECMDGOALS) || exit $$?; \
	done


.PHONY: all
all: subdirs

.PHONY: run
run: subdirs
	$(MAKE) report

.PHONY: rerun
rerun: run

.PHONY: install
install: subdirs

.PHONY: uninstall
uninstall: subdirs

.PHONY: clean
clean: subdirs

.PHONY: distclean
distclean: subdirs
	rm -f $(REPORT) $(SYSTEMINFO) $(SUMMARY)

.PHONY: prepare
prepare:
	chown -R root:root .
	chmod -R u=rwX,go=rX .
	restorecon -RF .

.PHONY: report
report: systeminfo summary
	@tarball="logs-$$(date +'%m%d%Y_%H%M').tar.xz"; \
	logs="audit-test/*/run.log audit-test/*/rollup.log"; \
	logs+=" audit-test/audit.*.log ltp/ltp.*.log"; \
	logs+=" audit-test/envcheck.log systeminfo.run.log"; \
	eval tar --ignore-failed-read -cvJf "$$tarball" "$$logs"; \
	ls -l "$$tarball"

.PHONY: systeminfo
systeminfo:
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

.PHONY: summary
summary:
	@echo "Test Report Summary ($$(date))" > $(SUMMARY)
	@echo "" >> $(SUMMARY)
	@for dir_iter in $(SUB_DIRS); do \
		if ls $$dir_iter/*rollup.log > /dev/null 2>&1; then \
			log_iter=$$(ls $$dir_iter/*rollup.log); \
		elif ls $$dir_iter/run.log > /dev/null 2>&1; then \
			log_iter=$$(ls $$dir_iter/run.log); \
		else \
			continue; \
		fi; \
		echo " looking at report: $$log_iter"; \
		echo "####### Log File: $$log_iter" >> $(SUMMARY); \
		cat $$log_iter >> $(SUMMARY); \
		echo "####### Log End" >> $(SUMMARY); \
		echo "" >> $(SUMMARY); \
	done

.PHONY: dist
dist:
	tar --owner root --group root --mode u=rwX,go=rX \
		-cvzf audit-test.tar.gz $(SUB_DIRS) $(EXTRA_DIST_FILES)
