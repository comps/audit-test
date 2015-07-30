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

TOPDIR		= ../..

include $(TOPDIR)/rules.mk

#######
#
# LSPP test Makefile
#
#

SELINUX_POLICY_TYPE := mls
SELINUX_POLICY_DEFTYPEFILE := /etc/selinux/$(SELINUX_POLICY_TYPE)/contexts/default_type
SELINUX_POLICY_STAFFUCONTEXTS := /etc/selinux/$(SELINUX_POLICY_TYPE)/contexts/users/staff_u
TEST_STAFFUCONTEXTS := "lspp_test_r:local_login_t:s0       staff_r:staff_t:s0 sysadm_r:sysadm_t:s0"

TEST_BASEDIR := $(shell realpath $(TOPDIR))
SELINUX_DEV_MAKEFILE := /usr/share/selinux/devel/Makefile

SELINUX_USER := staff_u

MODULE_NAME := lspp_test
MODULE_ROLE := lspp_test_r
MODULE_DEFTYPE := lspp_harness_t

###
#
# targets
#

# only in selinux mls mode / lspp profile
ifeq ($(PPROFILE),lspp)

all: $(MODULE_NAME).pp

$(MODULE_NAME).pp:
	$(MAKE) -f $(SELINUX_DEV_MAKEFILE)

verify:
	@echo -n " LSPP test policy module version: "; \
	if semodule -l | grep -q $(MODULE_NAME); then \
		semodule -l | grep $(MODULE_NAME) | cut -f 2; \
	else \
		echo "not installed"; \
	fi;
	@echo -n " Checking test login context: "; \
	if grep -q $(MODULE_ROLE) $(SELINUX_POLICY_STAFFUCONTEXTS); then \
		echo "installed"; \
	else \
		echo "not installed"; \
	fi;
	@echo -n " Number of LSPP test files labeled incorrectly: "; \
	restorecon -Frvn $(TEST_BASEDIR) | wc -l;

# During this install a role is added to an SELinux user which we use 
# as a positive test of semanage.  If this did not work correctly the 
# testing infrastructure would not run.

install: $(MODULE_NAME).pp
	@echo "$$(id -Z)" | egrep -q "(secadm_r|lspp_test_r)" || { echo \
		"Notice: secadm_r role is needed to install/upgrade SELinux policy module"; \
		newrole -r secadm_r -- -c "$(MAKE) install"; exit $$?; }; \
	if semodule -l | grep -q $(MODULE_NAME); then \
		echo -n "Upgrading $(MODULE_NAME) module ... "; \
		semodule -u $(MODULE_NAME).pp &> /dev/null && echo "ok" || \
			echo "failed (check the policy version number)"; \
	else \
		echo -n "Loading $(MODULE_NAME) module ... "; \
		semodule -i $(MODULE_NAME).pp &> /dev/null && echo "ok" || \
			echo "failed"; \
		range='s0-s15:c0.c1023'; \
		existing_roles=$$(semanage user -l | grep "^$(SELINUX_USER)" | awk 'sub("^.*[ \t]"$$5,$$5)'); \
		if [ "$$existing_roles" ]; then \
			echo -n "Adding additional role \"$(MODULE_ROLE)\" to user \"$(SELINUX_USER)\" ... "; \
			semanage user -m -r "$$range" -R "$$existing_roles $(MODULE_ROLE)" $(SELINUX_USER) &> /dev/null && \
				echo "ok" || \
				echo "failed (check the SELinux user)"; \
			echo "Adding role/type to the default_types file"; \
			echo $(MODULE_ROLE):$(MODULE_DEFTYPE) >> $(SELINUX_POLICY_DEFTYPEFILE); \
		fi; \
	fi; \
	echo -n "Adding test login context ..."; \
	if grep -q $(MODULE_ROLE) $(SELINUX_POLICY_STAFFUCONTEXTS); then \
		echo "already installed"; \
	else \
		echo $(TEST_STAFFUCONTEXTS) >> $(SELINUX_POLICY_STAFFUCONTEXTS);\
		echo "added"; \
	fi; \
	$(MAKE) relabel

uninstall:
	@echo "$$(ps -AZ)" | grep -q "lspp_test_r" && { echo \
		"Error: lspp_test_r is used by active sessions, cannot remove"; exit 1; } || true
	@echo "$$(id -Z)" | grep -q "secadm_r" || { echo \
		"Notice: secadm_r role is needed to uninstall SELinux policy module"; \
		en=$$(getenforce | grep -q Enforcing && echo 1 || echo 0); \
		newrole -r secadm_r -- -c "setenforce 0; $(MAKE) uninstall; setenforce $$en"; exit $$?; }; \
	echo "Removing role/type from the default_types file"; \
	sed -i -e '/$(MODULE_ROLE):$(MODULE_DEFTYPE)/d' $(SELINUX_POLICY_DEFTYPEFILE); \
	existing_roles=$$(semanage user -l | grep "^$(SELINUX_USER)" | awk 'sub("^.*[ \t]"$$5,$$5)'); \
	new_roles=$$(echo $$existing_roles | sed 's/$(MODULE_ROLE)//'); \
	echo -n "Removing $(MODULE_ROLE) from user \"$(SELINUX_USER)\" ... "; \
	semanage user -m -R "$$new_roles" $(SELINUX_USER) &> /dev/null && \
		echo "ok" || echo "failed"; \
	echo -n "Unloading $(MODULE_NAME) module ... "; \
	semodule -r $(MODULE_NAME) &> /dev/null && echo "ok" || echo "failed"; \
	echo -n "Removing test login context ...";  \
	if grep -q $(MODULE_ROLE) $(SELINUX_POLICY_STAFFUCONTEXTS); then \
		sed -i -e /$(MODULE_ROLE)/d $(SELINUX_POLICY_STAFFUCONTEXTS); \
		echo "removed"; \
	else \
		echo "not installed"; \
	fi; \
	$(MAKE) relabel

.PHONY: relabel
relabel:
	@echo "Relabeling LSPP tests"
	@restorecon -Fr $(TEST_BASEDIR)

clean:
	$(MAKE) -f $(SELINUX_DEV_MAKEFILE) clean

# remove only generated files
distclean:
	@shopt -s nullglob; \
	for file in *.fc; do \
		[ -z "$$(cat $$file)" ] && rm -vf "$$file"; \
	done; \
	for file in *.if; do \
		[ -z "$$(grep -v '<summary></summary>' < $$file)" ] && rm -vf "$$file"; \
	done;

endif
