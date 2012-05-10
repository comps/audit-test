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
# Verify audit of failed ssh.

source pam_functions.bash || exit 2
source tp_ssh_functions.bash || exit 2
disable_ssh_strong_rng

# test
expect -c "
    spawn ssh $TEST_USER@localhost
    expect {
       {continue} {send yes\r; exp_continue}
       {assword} {send badpassword\r}
    }
    expect {
        {permission denied} {close; wait}
        {assword} {close; wait}
    }"

msg_1="acct=\"*$TEST_USER\"*[ :]* exe=./usr/sbin/sshd.*terminal=ssh res=failed.*"
augrok -q type=USER_AUTH msg_1=~"PAM: *authentication $msg_1" || exit_fail

exit_pass
