###############################################################################
#   Copyright (c) 2015 Red Hat, Inc. All rights reserved.
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

install: install_setrans install_netlabel install_ipsec
uninstall: uninstall_setrans uninstall_netlabel uninstall_ipsec

.PHONY: install_setrans uninstall_setrans
install_setrans:
	cp -an /etc/selinux/mls/setrans.conf \
		/etc/selinux/mls/setrans.conf.audit-test-backup && \
		grep -q 'setrans_lspp.conf' /etc/selinux/mls/setrans.conf || \
			echo 'Include=/etc/selinux/mls/setrans.d/setrans_lspp.conf' \
				>> /etc/selinux/mls/setrans.conf
	install -o root -g root -m 640 setrans_lspp.conf /etc/selinux/mls/setrans.d
	chkconfig mcstransd on
	service mcstransd restart
uninstall_setrans:
	rm -f /etc/selinux/mls/setrans.d/setrans_lspp.conf
	mv -f /etc/selinux/mls/setrans.conf.audit-test-backup \
		/etc/selinux/mls/setrans.conf
	service mcstransd restart

.PHONY: install_netlabel uninstall_netlabel
install_netlabel:
	cp -an /etc/netlabel.rules /etc/netlabel.rules.audit-test-backup && \
		cat netlabel.rules > /etc/netlabel.rules
	chkconfig netlabel on
	service netlabel restart
uninstall_netlabel:
	mv -f /etc/netlabel.rules.audit-test-backup /etc/netlabel.rules
	service netlabel restart

.PHONY: install_ipsec uninstall_ipsec
install_ipsec:
	cp -an /etc/ipsec.conf /etc/ipsec.conf.audit-test-backup && \
		echo -e 'version 2.0\ninclude /etc/ipsec.d/audit-test-*.conf' > /etc/ipsec.conf
	cp -an /etc/ipsec.secrets /etc/ipsec.secrets.audit-test-backup && \
		echo 'include /etc/ipsec.d/audit-test-*.secrets' > /etc/ipsec.secrets
	chkconfig ipsec on
	systemctl restart ipsec
uninstall_ipsec:
	mv -f /etc/ipsec.conf.audit-test-backup /etc/ipsec.conf
	mv -f /etc/ipsec.secrets.audit-test-backup /etc/ipsec.secrets
	rm -f /etc/ipsec.d/audit-test-*.*
	systemctl restart ipsec
