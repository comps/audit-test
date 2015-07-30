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
#  PURPOSE: Downloads and runs specific tests from the LTP suite
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

LTP_TARBALL = ltp-full.tar.bz2
LTP_SRCURL = https://github.com/linux-test-project/ltp/archive/20150119.tar.gz
LTP_DESTDIR = /opt/ltp

# set temporary directory for tests
# this is because the default /tmp is mounted noexec in evaluated configuration
LTP_TMPDIR = $(LTP_DESTDIR)/tmp

# save allow_execmem / msgmni
ORIG_EXECMEM := $(shell getsebool deny_execmem | cut -d\  -f3)
ORIG_MSGMNI := $(shell sysctl -n kernel.msgmni)

# runtest files to run - comma-separated list
# - provided by upstream ltp
RUNTESTS := syscalls fs_perms_simple
# - additional from runtest/
RUNTESTS += $(wildcard $(CURDIR)/runtest/*)
# transform into comma-separated list
space :=
space +=
comma := ,
RUNTESTS := $(subst $(space),$(comma),$(RUNTESTS))

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
	mv -f ltp-[0-9]* $(LTP_BASE)

.PHONY: patch
patch:
	@for patchfile in $(PATCH_DIR)/*.patch; do \
		patch --batch --forward -p1 -d $(LTP_BASE) < "$$patchfile"; \
	done

.PHONY: build
build:
	@cd $(LTP_BASE) && \
	make autotools && \
	./configure --prefix=$(LTP_DESTDIR) && \
	make

.PHONY: install
install:
	@cd $(LTP_BASE) && \
	umask 022 && \
	make install

.PHONY: uninstall
uninstall:
	rm -rf $(LTP_DESTDIR)

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
	$(LTP_DESTDIR)/runltp -f $(RUNTESTS) -p -d $(LTP_TMPDIR) -l $(CURDIR)/ltp.rollup.log 2>&1 | tee ltp.run.log
	# cleanup
	setsebool deny_execmem=$(ORIG_EXECMEM)
	sysctl kernel.msgmni=$(ORIG_MSGMNI)
	faillock --reset

.PHONY: report
report:
	@{ \
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
	rm -f ltp.rollup.log ltp.run.log
	# remove downloaded and created files/dirs
	rm -f $(LTP_TARBALL)
	rm -rf $(LTP_BASE)

.PHONY: distclean
distclean: clean
