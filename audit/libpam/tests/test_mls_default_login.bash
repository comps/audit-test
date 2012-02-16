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
# Verify audit of successful login including default subject context.
# The test also checks if assigning default role to the user gets audited.

source pam_functions.bash || exit 2

# setup
# allow TEST_USER to write to tmpfile
chmod 666 $localtmp

# if in LSPP mode, map the TEST_USER to staff_u
if [[ $PPROFILE == "lspp" ]]; then
	semanage login -d $TEST_USER
	semanage login -a -s staff_u $TEST_USER
	# the added users range is taken from the default range
	# of the user who is running semanage:
	# see https://bugzilla.redhat.com/show_bug.cgi?id=785678#c8
	def_range=s0-s15:c0.c1023
	def_context=staff_u:staff_r:staff_t:$def_range
	auid=$(id -u "$TEST_USER")
	append_cleanup user_cleanup
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

msg_1="acct=\"*$TEST_USER\"* exe=./bin/login.* terminal=pts/$pts res=success.*"
augrok -q type=USER_AUTH msg_1=~"PAM:authentication $msg_1" || exit_fail
augrok -q type=USER_ACCT msg_1=~"PAM:accounting $msg_1" || exit_fail
augrok -q type=USER_START msg_1=~"PAM:session_open $msg_1" auid=$auid \
	subj=$login_context || exit_fail
# Check for ROLE_ASSIGN event for testuser
augrok -q type=ROLE_ASSIGN msg_1=~"op=login-sename,role,range acct=\"$TEST_USER\" old-seuser=user_u old-role=user_r old-range=s0 new-seuser=staff_u new-role=auditadm_r,staff_r,lspp_test_r,secadm_r,sysadm_r new-range=$def_range" || exit_fail "ROLE_ASSIGN event does not match"
# Check for USER_ROLE_CHANGE for login command
augrok -q type=USER_ROLE_CHANGE msg_1=~"pam: default-context=$def_context selected-context=$def_context: exe=./bin/login.* terminal=pts/$pts res=success.*" auid=$auid || exit_fail "USER_ROLE_CHANGE does not match"

exit_pass
