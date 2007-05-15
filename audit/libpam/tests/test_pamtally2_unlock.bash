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
# Verify pam_tally2 will lock an account

source pam_functions.bash || exit 2

# setup
tuid=$(id -u $TEST_USER)
grep -q pam_tally2 /etc/pam.d/sshd || exit_error

/sbin/pam_tally2 -u $TEST_USER --reset=4 --quiet || exit_error
expect -c '
    spawn ssh $env(TEST_USER)@localhost
    expect -nocase {Are you sure you want to continue} {send "yes\r"}
    expect -nocase {password: $} {send "badpassword\r"}
    expect -nocase {permission denied} {close; wait}'

# test
/sbin/pam_tally2 -u $TEST_USER -r --quiet || exit_error

msg_1="pam_tally2 uid=$tuid reset=0: exe=./sbin/pam_tally2.*res=success.*"
augrok -q type=USER_ACCT msg_1=~"$msg_1" || exit_fail

# verify the account is unlocked
expect -c '
    spawn ssh $env(TEST_USER)@localhost
    expect -nocase {Are you sure you want to continue} {send "yes\r"}
    expect -nocase {password: $} {
        send "$env(TEST_USER_PASSWD)\r"
        send "PS1=:\\::\r"
    }
    expect {:::$} {close; wait}'

msg_2="acct=$TEST_USER : exe=./usr/sbin/sshd.*terminal=ssh res=success.*"
augrok -q type=CRED_REFR msg_1=~"PAM: setcred $msg_2" || exit_fail
augrok -q type=USER_ACCT msg_1=~"PAM: accounting $msg_2" || exit_fail

exit_pass
