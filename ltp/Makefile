##########################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
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
#  FILE   : Makefile
#
#  PURPOSE: Downloads and runs syscalls and cc_ospp tests from LTP suite
#
#  REQUIRENTS: Exported PASSWD env variable
#
#  HISTORY:
#    11/11 originated by Miroslav Vadkerti <mvadkert@redhat.com>
#
##########################################################################

TOPDIR		= .

include $(TOPDIR)/rules.mk

LTP_BASE = ltp-full
PATCH_DIR = patch
MDIR = $(CURDIR)

LTP_TARBALL = ltp-full.tar.bz2
LTP_SRCURL = http://sourceforge.net/projects/ltp/files/LTP%20Source/ltp-20140422/ltp-full-20140422.tar.bz2/download

# set temporary directory to /opt/tmp for tests
# this is because the default /tmp is mounted noexec in evaluated configuration
LTP_TMPDIR = /opt/ltp/tmp

# save allow_execmem / msgmni
ORIG_EXECMEM = $(shell getsebool deny_execmem | cut -d\  -f3)
ORIG_MSGMNI = $(shell sysctl -n kernel.msgmni)

all: download extract patch build
run: runtests report

.PHONY: download
download:
	if [ ! -e $(LTP_TARBALL) ]; then \
		wget -O $(LTP_TARBALL) $(LTP_SRCURL); \
	else \
		echo "LTP source exists - skipping download"; \
	fi

.PHONY: extract
extract:
	tar xvf $(LTP_TARBALL)
	mv -f ltp-full-* $(LTP_BASE)

.PHONY: patch
patch:
	cp -fv $(PATCH_DIR)/cc_commands $(LTP_BASE)/runtest/cc_commands
	cp -fv $(PATCH_DIR)/cc_ospp.sh $(LTP_BASE)/testscripts/cc_ospp.sh
	cp -fv $(PATCH_DIR)/mnt_syscalls.sh $(LTP_BASE)/testscripts/mnt_syscalls.sh
	cp -fv $(PATCH_DIR)/mnt_syscalls $(LTP_BASE)/runtest/mnt_syscalls
	cp -fv $(PATCH_DIR)/su01 $(LTP_BASE)/testcases/commands/su/su01
	cp -fv $(PATCH_DIR)/sync_file_range01.c $(LTP_BASE)/testcases/kernel/syscalls/sync_file_range/sync_file_range01.c
	cp -fv $(PATCH_DIR)/test_containers.sh $(LTP_BASE)/testscripts/test_containers.sh

.PHONY: build
build:
	@cd $(LTP_BASE); \
	./configure; \
	make; \
	make install

.PHONY: rerun
rerun:

.PHONY: runtests
runtests:
	# needed for sendfile tests
	setsebool deny_execmem=off
	# needed for msgctl11 to pass, see
	#  http://www.mail-archive.com/ltp-list@lists.sourceforge.net/msg12767.html
	sysctl kernel.msgmni=700
	# run tests
	echo "Running syscalls tests"
	/opt/ltp/testscripts/syscalls.sh -p -d ${LTP_TMPDIR} -l $(MDIR)/syscalls.rollup.log 2>&1 | tee syscalls.run.log
	echo "Running cc_ospp tests"
	/opt/ltp/testscripts/cc_ospp.sh -p -d ${LTP_TMPDIR} -l $(MDIR)/cc_ospp.rollup.log 2>&1 | tee cc_ospp.run.log
	echo "Running mnt_syscalls tests"
	/opt/ltp/testscripts/mnt_syscalls.sh -p -d ${LTP_TMPDIR} -l $(MDIR)/mnt_syscalls.rollup.log 2>&1 | tee mnt_syscalls.run.log
	echo "Running container tests"
	/opt/ltp/testscripts/test_containers.sh
	# cleanup
	setsebool deny_execmem=$(ORIG_EXECMEM)
	sysctl kernel.msgmni=$(ORIG_MSGMNI)

.PHONY: report
report:
	@for logtype in rollup run; do \
		for testset in syscalls cc_ospp mnt_syscalls containers; do \
			echo "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"; \
			printf "::: %-52s :::\n" "$$testset"; \
			echo "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"; \
			echo; \
			if [ -f "$$testset.$$logtype.log" ]; then cat "$$testset.$$logtype.log"; fi; \
			echo; \
		done &> ltp.$$logtype.log; \
	done; \
	{ \
		echo "Summary:"; \
		echo "--------"; \
		echo; \
		echo "TESTS PASSED = $$(sort -u < ltp.rollup.log | grep -c ' PASS ')"; \
		echo "      FAILED = $$(sort -u < ltp.rollup.log | grep -c ' FAIL ')"; \
		echo; \
		echo "Failed tests:"; \
		echo "-------------"; \
		cat ltp.rollup.log | grep ' FAIL '; \
	} 2>&1 >> ltp.rollup.log; \
	exit 0

.PHONY: clean
clean:
	# remove installed ltp
	rm -rf /opt/ltp
	# remove logs
	for logtype in rollup run; do \
		for testset in syscalls cc_ospp mnt_syscalls containers; do \
			rm -f "$$testset.$$logtype.log"; \
		done; \
	done; \
	rm -f ltp.rollup.log ltp.run.log
	# remove downloaded and created files/dirs
	rm -f $(LTP_TARBALL)
	rm -rf $(LTP_BASE)

.PHONY: dist
dist:
	rev=$$(git log | head -n 1| awk '/^commit/{print $$2}' | cut -b 1-6 ) && \
	tmpdir=$$(mktemp -d) && \
	into=$${PWD%/*} && \
	mkdir "$$tmpdir/ltp" && \
	rsync -a --exclude=.git . "$$tmpdir/ltp" && \
	$(MAKE) clean -C "$$tmpdir/ltp" && \
	cd "$$tmpdir" && \
	chmod -R a+rX ltp && \
	tar czf "$$into/ltp-$$rev.tar.gz" ltp && \
	cd "$$into" && \
	rm -rf "$$tmpdir" && \
	echo && \
	ls -l ltp-$$rev.tar.gz
