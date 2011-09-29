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

.PHONY: install getaddress \
	install_client install_server \
	install_check install_setrans install_netlabel \
	install_ipsec_client install_ipsec_server

# perform the client install by default
install: install_client

# helper target to get local addresses
getaddress: install_check
	@echo "Local IPv4 address -> %LOCAL_IPV4%" | ../addr_filter.bash
	@echo "Local IPv6 address -> %LOCAL_IPV6_RAW%" | ../addr_filter.bash

install_client: install_setrans install_ipsec_client install_netlabel
	cat rc.local.client | ../addr_filter.bash > rc.local
	install -o root -g root -m 755 rc.local /etc/rc.d
	if [[ ! -L /etc/rc3.d/S99local ]]; then \
		(cd /etc/rc3.d; ln -s ../rc.local S99local); \
	fi
	restorecon /etc

install_server: install_setrans install_ipsec_server install_netlabel
	if [[ ! -f client_list.txt ]]; then \
		echo "error: file client_list.txt does not exist"; \
		exit 1; \
	fi
	cat rc.local.server.in_header > rc.local
	cat client_list.txt | ../addr_loop.bash -4 rc.local.server.in_body >> rc.local
	cat rc.local.server.in_footer >> rc.local
	install -o root -g root -m 755 rc.local /etc/rc.d
	if [[ ! -L /etc/rc3.d/S99local ]]; then \
		(cd /etc/rc3.d; ln -s ../rc.local S99local); \
	fi
	restorecon /etc

install_check:
	@[[ -n $$LBLNET_SVR_IPV4 ]] || \
		(echo "error: variable LBLNET_SVR_IPV4 is not set"; /bin/false)
	@[[ -n $$LBLNET_SVR_IPV6 ]] || \
		(echo "error: variable LBLNET_SVR_IPV6 is not set"; /bin/false)

install_check_server:
	@[[ -n $$LBLNET_PREFIX_IPV6 ]] || \
		(echo "error: variable LBLNET_PREFIX_IPV6 is not set"; \
		 /bin/false)

install_setrans:
	grep -q "# labeled networking" /etc/selinux/mls/setrans.conf || \
	(echo ""; \
	 echo "# labeled networking"; \
	 echo "s15:c0.c239=NetHigh"; \
	 echo "s1:c1=NetS1"; \
	 echo "s1:c1.c3=NetIncomp1"; \
	 echo "s1:c4.c6=NetIncomp2"; \
	 echo "s0:c0=NetLowIncomp") >> /etc/selinux/mls/setrans.conf

install_netlabel:
	install -o root -g root -m 640 --backup=t netlabel.rules /etc
	chkconfig netlabel on

install_ipsec_client: install_check
	cat ipsec.conf.client | ../addr_filter.bash > ipsec.conf
	install -o root -g root -m 600 ipsec.conf /etc/ipsec.conf
	install -o root -g root -m 600 ipsec.secrets /etc/ipsec.secrets

install_ipsec_server: install_check_server
	if [[ ! -f client_list.txt ]]; then \
		echo "error: file client_list.txt does not exist"; \
		exit 1; \
	fi
	cat ipsec.conf.server.in_header > ipsec.conf
	cat client_list.txt | ../addr_loop.bash -4 ipsec.conf.server.in_body >> ipsec.conf
	install -o root -g root -m 600 ipsec.conf /etc/ipsec.conf
	install -o root -g root -m 600 ipsec.secrets /etc/ipsec.secrets
