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

include $(TOPDIR)/rules.mk

DIRS        = audit ltp

SUB_DIRS        = audit-test ltp

LOG		= run.log

run:
	for DIR in $(SUB_DIRS); do \
		make -C $$DIR run; \
	done
	make report

.PHONE: report
report: systeminfo summary
	@tarball="logs-$$(date +'%m%d%Y_%H%M').tar.gz"; \
	tar zcvf logs-$$(date +"%m%d%Y_%H%M").tar.gz $$(find . -name "*.log"); \
	ls -l $$tarball

.PHONY: summary
summary:
	@echo "Test Report Summary ($$(date))" > $(LOG)
	@echo "" >> $(LOG)
	@for dir_iter in $(SUB_DIRS); do \
		if ls $$dir_iter/*rollup.log > /dev/null 2>&1; then \
			log_iter=$$(ls $$dir_iter/*rollup.log); \
		elif ls $$dir_iter/run.log > /dev/null 2>&1; then \
			log_iter=$$(ls $$dir_iter/run.log); \
		else \
			continue; \
		fi; \
		echo " looking at report: $$log_iter"; \
		echo "####### Log File: $$log_iter" >> $(LOG); \
		cat $$log_iter >> $(LOG); \
		echo "####### Log End" >> $(LOG); \
		echo "" >> $(LOG); \
	done

.PHONY: dist
dist:
	rev=$$(git log | head -n 1| awk '/^commit/{print $$2}' | cut -b 1-6 ) && \
	tmpdir=$$(mktemp -d) && \
	into=$${PWD%/*} && \
	for DIR in $(DIRS); do make -C "$$DIR" dist; done && \
	mv "ltp-$$rev.tar.gz" "audit-test-$$rev.tar.gz" "$$tmpdir" && \
	cp "Makefile" "rules.mk" "$$tmpdir" && \
	cd "$$tmpdir" && \
	tar xzvf "ltp-$$rev.tar.gz" && \
	tar xzvf "audit-test-$$rev.tar.gz" && \
	tar czf "$$into/audit-$$rev.tar.gz" "Makefile" "rules.mk" "ltp" "audit-test" && \
	echo "$$into" && \
	cd "$$into" && \
	rm -rf "$$tmpdir" && \
	echo && \
	ls -l audit-$$rev.tar.gz
