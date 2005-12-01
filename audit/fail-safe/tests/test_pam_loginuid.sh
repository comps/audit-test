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

export RHOST="localhost"
# TEST_USER and TEST_USER_PASSWD are set in run.bash startup()

function cleanup {
  # restore original pam config
  mv /etc/pam.d/sshd.testsave /etc/pam.d/sshd
  killall -0 auditd &>/dev/null || /etc/init.d/auditd start
}
trap 'cleanup; exit' 0 1 2 3 15

# make sure pam_loginuid is configured with require_auditd
sed -i.testsave 's/^pam_loginuid\.so.*/& require_auditd/' /etc/pam.d/sshd \
    || exit 2

# attempt to login with auditd running; should work
./ssh01_s1 || exit 1

# attempt to login with auditd off; should fail
service auditd stop || exit 2
./ssh01_s1 && exit 1

# tests passed
exit 0
