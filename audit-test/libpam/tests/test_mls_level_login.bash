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

AUDITMARK=$(get_audit_mark)

# if in LSPP mode, map the TEST_USER to staff_u and give it a range
if [[ $PPROFILE == "lspp" ]]; then
	semanage login -d $TEST_USER	
	semanage login -a -s staff_u -r s0-s2 $TEST_USER
	# XXX should compute the default context from the policy
	def_context=staff_u:staff_r:staff_t:s0-s2
	sel_context=staff_u:staff_r:staff_t:s2
	auid=$(id -u "$TEST_USER")
	append_cleanup user_cleanup
else 
	exit_error "Not in lspp mode"
fi

# calling login in this manner leaves an entry in /var/run/utmp
# use backup (and automatic restore) to work around this
backup /var/run/utmp

# In RHEL7 the pam_loginuid fails if loginuid already set
# for the purpose of this test we disable it temporarily
backup /etc/pam.d/login
sed -i 's/\(^session.*pam_loginuid.*$\)/\#\1/' /etc/pam.d/login

# test
(
    export localtmp
    expect -c '
        spawn login
        sleep 1
        expect -nocase {login: $} {send "$env(TEST_USER)\r"}
        expect -nocase {password: $} {send "$env(TEST_USER_PASSWD)\r"}
	expect -nocase {level} {send "Y\r"}
	expect -nocase {role:} {send "\r"}
	expect -nocase {level:} {send "s2\r"}
        send "PS1=:\\::\r"
        expect {:::$} {send "printf \"sel_context2=%s\\npts=%s\\n\" `cat /proc/self/attr/current` `tty` > $env(localtmp)\r"}
        expect {:::$} {close; wait}'
)

source $localtmp

#verify that the context is what we asked for
if [[ $sel_context != $sel_context2 ]]; then
	exit_fail;
fi

augrok --seek=$AUDITMARK type=USER_START
msg_1="grantors=pam_selinux,pam_console,pam_selinux,pam_namespace,pam_keyinit,\
pam_keyinit,pam_limits,pam_systemd,pam_unix,pam_lastlog acct=\"$TEST_USER\" \
exe=\"/usr/bin/login\" hostname=? addr=? terminal=${pts#/dev/} res=success"
augrok --seek=$AUDITMARK type=USER_START msg_1="op=PAM:session_open $msg_1" subj=$login_context || exit_fail

# Check for USER_ROLE_CHANGE for login command
augrok --seek=$AUDITMARK type=USER_ROLE_CHANGE
augrok --seek=$AUDITMARK type=USER_ROLE_CHANGE msg_1=~"pam: default-context=$def_context selected-context=$sel_context.*exe=.(/usr)?/bin/login.* terminal=${pts#/dev/} res=success.*" || exit_fail "USER_ROLE_CHANGE does not match"

exit_pass
