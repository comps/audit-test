#!/bin/bash
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
# 
# PURPOSE:
# Verify audit of successful su.

if [[ $EUID == 0 ]]; then
    source pam_functions.bash || exit 2

    # setup
    # allow TEST_USER to write to tmpfile created by root
    chmod 666 $tmp1

    # test
    # rerun this script as TEST_USER.  Confine the exports to a subshell
    (
        export tmp1
        export TEST_EUID=$(id -u "$TEST_USER") zero=$0
        perl -MPOSIX -e 'setuid $ENV{TEST_EUID}; system $ENV{zero}'
    )

    # returned from test, collect results
    pts=$(<$tmp1)
    pts=${pts##*/}

    msg_1="acct=\"*$TEST_USER\"* : exe=./bin/su.*terminal=pts/$pts res=success.*"
    augrok -q type=USER_AUTH msg_1=~"PAM: authentication $msg_1" || exit_fail
    augrok -q type=USER_ACCT msg_1=~"PAM: accounting $msg_1" || exit_fail

    exit_pass
fi

# This is reached through the perl -e 'system...' above
expect -c '
    spawn /bin/su - $env(TEST_USER)
    expect -nocase {password: $} {
        send "$env(TEST_USER_PASSWD)\r"
        send "PS1=:\\::\r"
    }
    expect {:::$} {send "tty > $env(tmp1)\r"}
    expect {:::$} {close; wait}'
