#!/bin/bash -x
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Matt Anderson <mra@hp.com>
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
# =============================================================================
#
# FILE		: test_pam_loginuid.sh
#
# PURPOSE	: Used to test pam_loginuid and the require_auditd flag
#
# DESCRIPTION	: This test runs through the following steps
#	1. configure pam_loginuid with require_auditd
#	2. attempt to login with auditd running
#	3. attempt to login with auditd off
# 
# HISTORY	:
#  11/05 Initial version by Matt Anderson <mra@hp.com>
#  11/05 Mods to use global TEST_USER by Aron Griffis <aron@hp.com>
#

source testcase.bash

export RHOST="localhost"
# TEST_USER and TEST_USER_PASSWD are set in run.bash startup()

# setup
# make sure pam_loginuid is configured with require_auditd
backup /etc/pam.d/sshd  # restored automatically
sed -i '/pam_loginuid\.so/s/$/ require_auditd/' /etc/pam.d/sshd || exit_error
# make sure auditd is running after test
prepend_cleanup 'killall -0 auditd &>/dev/null || service auditd start'

# attempt to login with auditd running; should work
./ssh01_s1 || exit_fail

# attempt to login with auditd off; should fail
service auditd stop || exit_error
./ssh01_s1 && exit_exit_fail

exit_pass
