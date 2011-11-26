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
#
#
#  FILE   : Makefile
#
#  PURPOSE: Downloads and runs syscalls and cc_ospp tests from LTP suite
#
#  REQUIRENTS: Exported RHOST and PASSWD env variables
#
#  HISTORY:
#    11/11 originated by Miroslav Vadkerti <mvadkert@redhat.com>
#
##########################################################################

RUN_LOG = run.log
ROLLUP_LOG = rollup.log

LTP_BASE = ltp-full
LTP_BASE_PATH = $(CURDIR)/$(LTP_BASE)
PATCH_DIR = $(CURDIR)/patch
MDIR = $(CURDIR)

LTP_ROLLUP_LOG = .rollup.log
LTP_RUN_LOG = .run.log

LTP_FULL = ltp-full.tar.bz2

all:: download extract patch build

run:: runtests report

build::
	echo "Makefile: build"
	export TOP_SRCDIR=$(LTP_BASE_PATH)
	@$(MAKE) -C $(LTP_BASE_PATH) autotools
	cd $(LTP_BASE_PATH) && ./configure && cd -
	@$(MAKE) -C $(LTP_BASE_PATH) all
	@$(MAKE) -C $(LTP_BASE_PATH) install

extract::
	echo "Makefile: extract"
	tar xvf $(LTP_FULL)
	mv -f ltp-full-* $(LTP_BASE)

patch::
	echo "Makefile: patch"
	cp -fv $(PATCH_DIR)/cc_commands $(LTP_BASE_PATH)/runtest/cc_commands
	cp -fv $(PATCH_DIR)/cc_ospp.sh $(LTP_BASE_PATH)/testscripts/cc_ospp.sh
	cp -fv $(PATCH_DIR)/ssh03 $(LTP_BASE_PATH)/testcases/network/tcp_cmds/ssh/ssh03
	cp -fv $(PATCH_DIR)/su01 $(LTP_BASE_PATH)/testcases/commands/su/su01

clean::
	echo "Makefile: clean"
	# remove installed ltp
	rm -rf /opt/ltp
	# remove all logs, downloaded and created files/dirs
	rm -f cc_ospp$(LTP_ROLLUP_LOG) cc_ospp$(LTP_RUN_LOG)
	rm -f syscalls$(LTP_ROLLUP_LOG) syscalls$(LTP_RUN_LOG)
	rm -f $(LTP_FULL) $(RUN_LOG) $(ROLLUP_LOG)
	rm -rf $(LTP_BASE)

download::
	echo "Makefile: download"
	# download latest LTP source
	wget -O $(LTP_FULL) http://sourceforge.net/projects/ltp/files/latest/download

runtests::
	echo "Makefile: runtests"
	# needed for sendfile tests
	setsebool allow_execmem=on
	# run tests
	echo "Running syscalls tests"
	/opt/ltp/testscripts/syscalls.sh -p -l $(MDIR)/syscalls$(LTP_ROLLUP_LOG) 2>&1 | tee syscalls$(LTP_RUN_LOG)
	echo "Running cc_ospp tests"
	/opt/ltp/testscripts/cc_ospp.sh -p -l $(MDIR)/cc_ospp$(LTP_ROLLUP_LOG) 2>&1 | tee cc_ospp$(LTP_RUN_LOG)

report::
	echo "Makefile: report"
	# create rollup.log
	echo "TESTS PASSED = "$$(grep PASS cc_ospp$(LTP_ROLLUP_LOG) syscalls$(LTP_ROLLUP_LOG) 2>&1 | wc -l) > $(ROLLUP_LOG)
	echo "      FAILED =" $$(grep FAIL cc_ospp$(LTP_ROLLUP_LOG) syscalls$(LTP_ROLLUP_LOG) 2>&1 | wc -l) >> $(ROLLUP_LOG)
	echo >> $(ROLLUP_LOG)
	if [ "x$$(grep FAIL cc_ospp$(LTP_ROLLUP_LOG) syscalls$(LTP_ROLLUP_LOG) 2>&1 | wc -l)" != "x0" ]; then \
		echo "Failed tests:" >> $(ROLLUP_LOG); \
		echo "-------------" >> $(ROLLUP_LOG); \
		grep -H "\WFAIL\W" cc_ospp$(LTP_ROLLUP_LOG) syscalls$(LTP_ROLLUP_LOG) >> $(ROLLUP_LOG); \
	fi
	# create run.log
	cat cc_ospp$(LTP_RUN_LOG) syscalls$(LTP_RUN_LOG) > $(RUN_LOG)
