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
# Verify audit of successful vsftpd login.

source pam_functions.bash || exit 2

# setup
setsebool -P ftp_home_dir=1
# XXX: fd 63 is left open by something, causing the tests to hang
prepend_cleanup "service vsftpd restart 63>/dev/null"
prepend_cleanup "setsebool -P ftp_home_dir=0"
backup "$vsftpd_conf"
write_config \
	"$vsftpd_conf" \
	local_enable=YES

# XXX: fd 63 is left open by something, causing the tests to hang
service vsftpd restart 63>/dev/null

echo Made it this far

# test
expect -c '
    spawn ftp localhost
    expect -nocase {name} {send "$env(TEST_USER)\r"}
    expect -nocase {password:$} {send "$env(TEST_USER_PASSWD)\r"}
    expect {ftp> $} {send "quit\r"}'

msg_1="acct=\"$TEST_USER\" exe=./usr/sbin/vsftp.*hostname=(127.0.0.1|localhost.*) addr=(::1|127.0.0.1) terminal=ftp res=success.*"
augrok -q type=USER_AUTH msg_1=~"PAM:authentication $msg_1" || exit_fail
augrok -q type=USER_ACCT msg_1=~"PAM:accounting $msg_1" || exit_fail

exit_pass
