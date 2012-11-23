#!/bin/bash
#*********************************************************************
#   Copyright (C) International Business Machines  Corp., 2000
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
#   along with this pronram;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#  FILE   :test_ssh 04.bash
#
#  PURPOSE: Tests to see that ssh rejects a 'root' login attempt
#
#  SETUP: The program `/usr/bin/expect' MUST be installed.
#
#  HISTORY:
#    10/11 T.N Santhosh (santhosh.tn@hp.com)
#

source testcase.bash || exit 2
RUSER="root"

expect -c "
    spawn ssh root@localhost
	expect {
        {continue} {send yes\r; exp_continue}
        {assword} {send badpassword\r}
    }
    expect {
        {permission denied} {close; wait}
        {assword} {close; wait}
    }"

msg_1="acct=\"*$RUSER\"*[ :]* exe=./usr/sbin/sshd.*terminal=ssh res=failed.*"
augrok -q type=USER_AUTH  msg_1=~"PAM:authentication $msg_1"  || exit_fail

exit_pass
