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
# Verify audit of successful vsftpd login.

source pam_functions.bash || exit 2

# test
expect -c '
    spawn ftp localhost
    expect -nocase {name} {send "$env(TEST_USER)\r"}
    expect -nocase {password:$} {send "$env(TEST_USER_PASSWD)\r"}
    expect {ftp> $} {send "quit\r"}'

msg_1="acct=$TEST_USER : exe=./usr/sbin/vsftp.*hostname=localhost, addr=127.0.0.1, terminal=ftp res=success.*"
augrok -q type=USER_AUTH msg_1=~"PAM: authentication $msg_1" || exit_fail
augrok -q type=USER_ACCT msg_1=~"PAM: accounting $msg_1" || exit_fail

exit_pass
