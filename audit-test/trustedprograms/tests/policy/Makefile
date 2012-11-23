###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
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

#######
#
# SELinux policy tools test Makefile
#
#

#TOPDIR          = ../../..
#include $(TOPDIR)/rules.mk

SELINUX_POLICY_TYPE := mls
SELINUX_POLICY_RPM := selinux-policy-$(SELINUX_POLICY_TYPE)
SELINUX_DEV_RPM := selinux-policy-devel

SELINUX_POLICY_DEFTYPEFILE := /etc/selinux/$(SELINUX_POLICY_TYPE)/contexts/default_type
SELINUX_DEV_BASEDIR := /usr/share/selinux/devel


TEST_BASEDIR := /usr/local/eal4_testing

###
#
# targets
#
.PHONY: verifyme verify install uninstall relabel

# base SELinux module targets
include $(SELINUX_DEV_BASEDIR)/Makefile

verifyme: verify
verify:
	@echo "Checking system for basic requirements"
	@echo -n " SELinux policy RPM version ($(SELINUX_POLICY_RPM)): "; \
	if rpm -q $(SELINUX_POLICY_RPM) > /dev/null; then \
		rpm -q $(SELINUX_POLICY_RPM) --qf "%{VERSION}-%{RELEASE}"; \
		echo ""; \
	else \
		echo "not installed"; \
	fi;
	@echo -n " SELinux policy development RPM version: "; \
	if rpm -q $(SELINUX_DEV_RPM) > /dev/null; then \
		rpm -q $(SELINUX_DEV_RPM) --qf "%{VERSION}-%{RELEASE}"; \
		echo ""; \
	else \
		echo "not installed"; \
	fi;

