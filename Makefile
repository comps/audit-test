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
	rm -rf systeminfo

.PHONY: distclean
distclean: subdirs
	rm -rf logs-*.tar.xz systeminfo

.PHONY: prepare
prepare:
	chown -R root:root .
	chmod -R u=rwX,go=rX .
	restorecon -RF .

.PHONY: report
report: systeminfo
	@tarball="logs-$$(date +'%m%d%Y_%H%M').tar.xz"; \
	logs=; \
	logs+=" audit-test/*/logs/run.log.* audit-test/*/rollup.log"; \
	logs+=" audit-test/audit.rollup.log"; \
	logs+=" ltp/logs/ ltp/ltp.rollup.log"; \
	logs+=" audit-test/envcheck.log"; \
	logs+=" systeminfo/"; \
	eval tar --ignore-failed-read --owner root --group root \
		--mode u=rwX,go=rX -cJf "$$tarball" "$$logs"; \
	ls -lh "$$tarball"

.PHONY: systeminfo
systeminfo:
	@mkdir -p systeminfo; cd systeminfo || exit 1; \
	date > date.txt; \
	rpm -qa | sort > rpms.txt; \
	uname -a > uname.txt; \
	hostname --fqdn > host.txt; \
	cat /proc/cpuinfo > cpuinfo.txt; \
	cat /proc/meminfo > meminfo.txt; \
	sysctl -a > sysctl.txt; \
	which dmidecode &>/dev/null && dmidecode > dmidecode.txt; \
	which lspci &>/dev/null && lspci > lspci.txt; \
	{ ip addr show; echo -e "\n--\n"; ip route show; echo -e "\n--\n"; \
		ip link show; echo -e "\n--\n"; ip rule show; } > network.txt; \
	{ iptables-save; echo -e "\n--\n"; ip6tables-save; } > firewall.txt; \
	dmsetup table > dmsetup.txt; \
	cat /proc/self/mounts > mounts.txt; \
	df -h > df.txt;

.PHONY: dist
dist:
	tar --owner root --group root --mode u=rwX,go=rX \
		-cJf audit-test.tar.xz $(SUB_DIRS) $(EXTRA_DIST_FILES)
	@ls -lh audit-test.tar.xz
