#!/bin/bash -x
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Matt Anderson <mra@hp.com>
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
# =============================================================================
#
# PURPOSE:
# Verify that pam_loginuid.so allows logins when auditd is running, and denies
# them when it is not.
# 
# HISTORY:
#  11/05 Initial version by Matt Anderson <mra@hp.com>
#  11/05 Mods to use global TEST_USER by Aron Griffis <aron@hp.com>
#

source testcase.bash || exit 2

action=$1
auditd_active=$(pidof auditd)

# setup
# make sure pam_loginuid is configured with require_auditd
if grep "pam_loginuid.so" /etc/pam.d/sshd | grep -qv "require_auditd"; then
    backup /etc/pam.d/sshd  # restored automatically
    sed -i '/pam_loginuid\.so/s/$/ require_auditd/' /etc/pam.d/sshd || \
	exit_error

fi
# make sure auditd is running after test
prepend_cleanup 'pidof auditd &>/dev/null || service auditd start'

if [[ $action == "fail" && -n $auditd_active ]]; then
    service auditd stop || exit_error
fi

# TEST_USER and TEST_USER_PASSWD are exported in run.bash startup()
expect -c '
    spawn ssh \
    -o "PubkeyAuthentication no" \
    -o "NoHostAuthenticationForLocalhost yes" \
    -l $env(TEST_USER) localhost whoami
    expect -nocase {password: $} {
        send "$env(TEST_USER_PASSWD)\r"
    }
    expect "$env(TEST_USER)" {exit 0}
    exit 1'

case $?:$action in
    0:success|1:fail)
        exit_pass ;;
    1:success|0:fail)
        exit_fail ;;
    *)
        exit_error ;;
esac

if [[ $action == "fail" && -n $auditd_active ]]; then
    service auditd start
fi
