###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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

# XXX - all this stuff for the config files is a kludge and needs cleanup

.PHONY: install \
	install_client install_server \
	install_check install_setrans install_netlabel \
	install_ipsec_client install_ipsec_server

# perform the client install by default
install: install_client

install_client: install_setrans install_ipsec_client install_netlabel
install_server: install_setrans install_ipsec_server install_netlabel

install_check:
	@[[ -n $$LBLNET_SVR_IPV4 ]] || \
		(echo "error: variable LBLNET_SVR_IPV4 is not set"; /bin/false)
	@[[ -n $$LBLNET_SVR_IPV6 ]] || \
		(echo "error: variable LBLNET_SVR_IPV6 is not set"; /bin/false)

install_setrans:
	install -o root -g root -m 640 setrans_lspp.conf /etc/selinux/mls/setrans.d
	grep -q 'setrans_lspp.conf' /etc/selinux/mls/setrans.conf || \
		echo 'Include=/etc/selinux/mls/setrans.d/setrans_lspp.conf' \
			>> /etc/selinux/mls/setrans.conf
	chkconfig mcstransd on
	service mcstransd restart

install_netlabel:
	install -o root -g root -m 640 --backup=t netlabel.rules /etc
	chkconfig netlabel on
	service netlabel restart

install_ipsec_client: install_check
	cat ipsec.conf.client | ./addr_filter.bash > ipsec.conf
	install -o root -g root -m 600 ipsec.conf /etc/ipsec.conf
	install -o root -g root -m 600 ipsec.secrets /etc/ipsec.secrets
	chkconfig ipsec on
	service ipsec restart

install_ipsec_server:
	if [[ ! -f client_list.txt ]]; then \
		echo "error: file client_list.txt does not exist"; \
		exit 1; \
	fi
	cat ipsec.conf.server.in_header > ipsec.conf
	cat client_list.txt | ./addr_loop.bash ipsec.conf.server.in_body >> ipsec.conf
	install -o root -g root -m 600 ipsec.conf /etc/ipsec.conf
	install -o root -g root -m 600 ipsec.secrets /etc/ipsec.secrets
	chkconfig ipsec on
	service ipsec restart
