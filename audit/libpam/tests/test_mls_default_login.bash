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
# Verify audit of successful login including default subject context.

source pam_functions.bash || exit 2

# setup
# allow TEST_USER to write to tmpfile
chmod 666 $localtmp

# if in LSPP mode, map the TEST_USER to staff_u
if [[ $PPROFILE == "lspp" ]]; then
	semanage login -d $TEST_USER
	semanage login -a -s staff_u $TEST_USER
	# XXX should compute the default context from the policy
	def_context=staff_u:sysadm_r:sysadm_t:s0
	auid=$(id -u "$TEST_USER")
else 
	exit_error "Not in lspp mode"
fi

# calling login in this manner leaves an entry in /var/run/utmp
# use backup (and automatic restore) to work around this
backup /var/run/utmp

# test
(
    export localtmp
    expect -c '
        spawn login
        expect -nocase {login: $} {send "$env(TEST_USER)\r"}
        expect -nocase {password: $} {send "$env(TEST_USER_PASSWD)\r"}
	expect -nocase {level} {send "\r"}
        send "PS1=:\\::\r"
        expect {:::$} {send "tty > $env(localtmp)\r"}
        expect {:::$} {close; wait}'
)

pts=$(<$localtmp)
pts=${pts##*/}

msg_1="acct=\"*$TEST_USER\"* : exe=./bin/login.* terminal=pts/$pts res=success.*"
augrok -q type=USER_AUTH msg_1=~"PAM: authentication $msg_1" || exit_fail
augrok -q type=USER_ACCT msg_1=~"PAM: accounting $msg_1" || exit_fail
augrok -q type=USER_START msg_1=~"PAM: session open $msg_1" auid=$auid \
	subj=$login_context || exit_fail
augrok -q type=USER_ROLE_CHANGE msg_1=~"pam: default-context=$def_context selected-context=$def_context: exe=./bin/login.* terminal=pts/$pts res=success.*" auid=$auid || exit_fail
exit_pass
