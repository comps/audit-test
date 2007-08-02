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
# Verify that with polyinstantiation enabled and configured for /tmp, users 
# get separate /tmp directories for each level that they log in as.
# Procedure:
# - Use semanage to configure the test user for s0-s2
# - Setup the PAM namespace configuration for /tmp
# - Cleanup any old /tmp files for the test user
# - Audit open syscalls by the test user
# - Log in as the test user at s0, create a file in /tmp, write the
#   user's security context into it and log out.
# - Verify the information in the audit record from creating the file.
#   The name should be relative to the namespace.
# - Log in as the test user at s2, create a file with the same name in /tmp, 
#   write the user's security context into it and log out.
# - Verify the information in the audit record from creating the file.
#   The name should be relative to the namespace.
# - From the test harness, verify that the two files are found in 
#   the instance directories and  are different.

source pam_functions.bash || exit 2

# if in LSPP mode, map the TEST_USER to staff_u and give it a range
if [[ $PPROFILE == "lspp" ]]; then
	semanage login -d $TEST_USER	
	semanage login -a -s staff_u -r s0-s2 $TEST_USER
	# XXX should compute the context from the policy
	s0_context=staff_u:sysadm_r:sysadm_t:SystemLow
	s2_context=staff_u:sysadm_r:sysadm_t:Secret
	s0_obj=staff_u:object_r:sysadm_tmp_t:s0
	s2_obj=staff_u:object_r:sysadm_tmp_t:s2
	auid=$(id -u "$TEST_USER")
else 
	exit_error "Not in lspp mode"
fi

# calling login in this manner leaves an entry in /var/run/utmp
# use backup (and automatic restore) to work around this
backup /var/run/utmp

# backup namespace.conf and configured it for the test case
backup /etc/security/namespace.conf
echo "/tmp /tmp-inst/ level root,adm" > /etc/security/namespace.conf

tmpinstdir=/tmp-inst
tmpnewfile=/tmp/newfile

# cleanup old /tmp files 
rm -rf $tmpinstdir/*_$TEST_USER

# Force the audit log to rotate; add our rule.
rotate_audit_logs || exit_error "log rotate failed"
prepend_cleanup "auditctl -D"
auditctl -a entry,always ${MODE:+-F arch=b$MODE} -S open -F uid=$auid || \
	exit_error "audit rule failed"

# Login as s0 and write the user's context to a file in /tmp.
(
    export tmpnewfile
    expect -c '
        spawn login
        expect -nocase {login: $} {send "$env(TEST_USER)\r"}
        expect -nocase {password: $} {send "$env(TEST_USER_PASSWD)\r"}
	expect -nocase {level} {send "Y\r"}
	expect -nocase {role:} {send "\r"}
	expect -nocase {level:} {send "s0\r"}
        send "PS1=:\\::\r"
	expect {:::$} {send "id -Z > $env(tmpnewfile)\r"}
        expect {:::$} {close; wait}'
)
# Check the path and context in the audit record.
augrok type==SYSCALL \
	subj=$s0_context auid=$auid success=yes \
	name=$tmpnewfile obj=$s0_obj\
	|| exit_fail "missing audit record"

log_mark=$(stat -c %s $audit_log)
# Login at s2 and write the user's context to a file in /tmp
# Also write the user's context into localtmp.
(
    export tmpnewfile
    expect -c '
        spawn login
        expect -nocase {login: $} {send "$env(TEST_USER)\r"}
        expect -nocase {password: $} {send "$env(TEST_USER_PASSWD)\r"}
	expect -nocase {level} {send "Y\r"}
	expect -nocase {role:} {send "\r"}
	expect -nocase {level:} {send "s2\r"}
        send "PS1=:\\::\r"
	expect {:::$} {send "id -Z > $env(tmpnewfile)\r"}
        expect {:::$} {close; wait}'
)

# Check the path and context in the audit record.
augrok --seek=$log_mark type==SYSCALL \
	subj=$s2_context auid=$auid success=yes \
	name=$tmpnewfile obj=$s2_obj\
	|| exit_fail "missing audit record"

# verify that the files created by each login are different
diff $tmpinstdir/*s0_$TEST_USER/newfile $tmpinstdir/*s2_$TEST_USER/newfile
[[ $? == 0 ]] && exit_fail "Files match unexpectedly"

exit_pass
