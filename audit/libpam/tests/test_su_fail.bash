#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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
###############################################################################
# 
# PURPOSE:
# Verify audit of failed su.

if [[ $EUID == 0 ]]; then
    source pam_functions.bash || exit 2

    # test -- reruns this script as TEST_USER
    TEST_USER=$TEST_USER TEST_USER_PASSWD=badpassword \
    TEST_EUID=$(id -u "$TEST_USER") tmp1=$tmp1 zero=$0 \
    perl -MPOSIX -e 'setuid $ENV{TEST_EUID}; system $ENV{zero}'

    msg_1="acct=$TEST_USER : exe=./bin/su.*res=failed.*"
    augrok -q type=USER_AUTH msg_1=~"PAM: authentication $msg_1" || exit_fail

    exit_pass
fi

expect -c '
    spawn /bin/su - $env(TEST_USER)
    expect -nocase {password: $} {send "$env(TEST_USER_PASSWD)\r"}
    expect {incorrect password} {close; wait}'
