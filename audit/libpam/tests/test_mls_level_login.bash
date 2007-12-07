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
# Verify audit of login when user successfully selects a non-default context.
# User is allowed a range of s0-s2 and picks s2.

source pam_functions.bash || exit 2

# setup
# allow TEST_USER to write to tmpfile
chmod 666 $localtmp

# if in LSPP mode, map the TEST_USER to staff_u and give it a range
if [[ $PPROFILE == "lspp" ]]; then
	semanage login -d $TEST_USER	
	semanage login -a -s staff_u -r s0-s2 $TEST_USER
	# XXX should compute the default context from the policy
	def_context=staff_u:sysadm_r:sysadm_t:s0-s2
	sel_context=staff_u:sysadm_r:sysadm_t:s2
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
	expect -nocase {level} {send "Y\r"}
	expect -nocase {role:} {send "\r"}
	expect -nocase {level:} {send "s2\r"}
        send "PS1=:\\::\r"
        expect {:::$} {send "printf \"pts=%s\\n\" `tty` > $env(localtmp)\r"}
        expect {:::$} {send "printf \"sel_context2=%s\\n\" `cat /proc/self/attr/current` >> $env(localtmp)\r"}
        expect {:::$} {close; wait}'
)

source $localtmp

#verify that the context is what we asked for
if [[ $sel_context != $sel_context2 ]]; then
	exit_fail;
fi

msg_1="acct=\"*$TEST_USER\"* : exe=./bin/login.* res=success.*"
augrok -q type=USER_AUTH msg_1=~"PAM: authentication $msg_1" || exit_fail
augrok -q type=USER_ACCT msg_1=~"PAM: accounting $msg_1" || exit_fail
augrok -q type=USER_START msg_1=~"PAM: session open $msg_1" auid=$auid \
	subj=$login_context || exit_fail
augrok -q type=USER_ROLE_CHANGE msg_1=~"pam: default-context=$def_context selected-context=$sel_context: exe=./bin/login.* res=success.*" auid=$auid || exit_fail
exit_pass
