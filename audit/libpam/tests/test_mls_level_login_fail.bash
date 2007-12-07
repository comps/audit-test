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
# Verify audit of failed login when user selects and invalid level.
# User is only allowed s0 but picks s15.

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
	sel_context=staff_u:sysadm_r:sysadm_t:s15
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
	expect -nocase {level:} {send "s15\r"}
	expect -nocase {"authentication failure"} {close; wait}'
)

msg_1="acct=\"*$TEST_USER\"* : exe=./bin/login.* res=failed.*"
augrok -q type=USER_START msg_1=~"PAM: session open $msg_1" auid=$auid \
	subj=$login_context || exit_fail 
augrok -q type=USER_ROLE_CHANGE msg_1=~"pam: default-context=$def_context selected-context=$sel_context: exe=./bin/login.* res=failed.*" auid=$auid || exit_fail
exit_pass
