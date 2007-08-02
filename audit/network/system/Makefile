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
	install -o root -g root -m 755 rc.local /etc/rc.d
	if [[ ! -L /etc/rc3.d/S99local ]]; then \
		(cd /etc/rc3.d; ln -s ../rc.local S99local); \
	fi
	restorecon /etc

install_server: install_setrans install_ipsec_server install_netlabel
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
	 echo "s0:c0=NetLowIncomp") >> /etc/selinux/mls/setrans.conf

install_netlabel:
	install -o root -g root -m 640 netlabel.rules /etc
	chkconfig netlabel on

install_ipsec_client: install_check
	cat setkey.conf.in | ../addr_filter.bash > setkey.conf
	cat psk.txt.in | ../addr_filter.bash > psk.txt
	install -o root -g root -m 600 racoon.conf /etc/racoon
	install -o root -g root -m 600 psk.txt /etc/racoon
	install -o root -g root -m 600 setkey.conf /etc/racoon

install_ipsec_server: install_check_server
	if [[ ! -f client_list.txt ]]; then \
		echo "error: file client_list.txt does not exist"; \
		exit 1; \
	fi
	cat setkey.conf.in_header > setkey.conf
	cat client_list.txt | \
		../addr_loop.bash -4 setkey.conf.in_ipv4 \
	                          -6 setkey.conf.in_ipv6 >> \
		setkey.conf
	cat setkey.conf.in_footer >> setkey.conf
	cat psk.txt.in_header > psk.txt
	cat client_list.txt | ../addr_loop.bash -A psk.txt.in_body >> psk.txt
	cat psk.txt.in_footer >> psk.txt
	install -o root -g root -m 600 racoon.conf /etc/racoon
	install -o root -g root -m 600 psk.txt /etc/racoon
	install -o root -g root -m 600 setkey.conf /etc/racoon
