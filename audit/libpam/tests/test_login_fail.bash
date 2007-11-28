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
# Verify audit of failed login.

source pam_functions.bash || exit 2

# test
expect -c '
    spawn login
    expect -nocase {login: $} {send "$env(TEST_USER)\r"}
    expect -nocase {password: $} {send "badpassword\r"}
    expect -nocase {login incorrect} {close; wait}'

augrok -q type=USER_AUTH msg_1=~"PAM: authentication acct=\"*$TEST_USER\"* : exe=./bin/login.* terminal=pts/.*res=failed.*" || exit_fail

exit_pass
