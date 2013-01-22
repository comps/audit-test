#!/usr/bin/env bash
#
#   Copyright 2010, 2011 International Business Machines Corp.
#   Copyright 2010, 2011 Ramon de Carvalho Valle
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#   test_network_export_source_ip.bash
#
#   Assert data originating from the processes representing virtual machine
#   environments using the network has its associtated source IP addresses.


source testcase.bash || exit 2

set -x

# clean all iptables rules at the end of the testing
append_cleanup "iptables-save | xtables_empty | iptables-restore"

for i in $(seq $first $last); do

	# clean all iptables rules
	iptables-save | xtables_empty | iptables-restore

	#   Check the host IP address of the virtual network associated with the
	#   virtual machine environment.

	eval "ping -c 5 -I \$kvm_guest_${i}_hostaddr \$kvm_guest_${i}_hostaddr"

	if [[ $? -ne 0 ]]; then
		exit_fail
	fi

	#   Wait the specified timeout (total time, in minutes) for the
	#   virtual machine environment network service to start.

	for i in $(seq 1 $kvm_guest_timeout); do
		eval "ping -c 5 -I \$kvm_guest_${i}_hostaddr \$kvm_guest_${i}_addr"

		if [[ $? -eq 0 ]]; then
			break
		fi

		sleep 60
	done

	log_prefix=$(tr -cd 0-9A-Za-z < /dev/urandom | head -c 27)

	eval "iptables -A INPUT -s \$kvm_guest_${i}_addr -p icmp --icmp-type echo-reply -j LOG --log-level=1 --log-prefix=\"$log_prefix: \""
	eval "iptables -A INPUT -s \$kvm_guest_${i}_addr -p icmp --icmp-type echo-reply -j DROP"

	#   Check the host IP address of the virtual network associated with the
	#   virtual machine environment after adding netfilter rules.

	eval "ping -c 5 -I \$kvm_guest_${i}_hostaddr \$kvm_guest_${i}_hostaddr"

	if [[ $? -ne 0 ]]; then
		exit_fail "Cannot ping guest host address"
	fi

	eval "ping -c 5 -I \$kvm_guest_${i}_hostaddr \$kvm_guest_${i}_addr"

	if [[ $? -eq 0 ]]; then
		exit_fail "Cannot ping guest address"
	fi

	log_count=$(eval "grep -c -E \"$log_prefix: .* SRC=\$kvm_guest_${i}_addr\" /var/log/messages")

	if [[ $log_count -eq 0 ]]; then
		exit_fail "log count is 0"
	fi
done

exit_pass

#   vim: set noet sw=8 ts=8 tw=0:
