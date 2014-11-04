#!/bin/bash
#*********************************************************************
#   Copyright (C) International Business Machines  Corp., 2000
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
#  FILE   : test_ssh_login_root.bash
#
#  PURPOSE: Tests to see that ssh rejects a 'root' login attempt with correct
#           and incorrect password
#
#  SETUP: The program `/usr/bin/expect' MUST be installed.
#
#  HISTORY:
#    10/11 T.N Santhosh (santhosh.tn@hp.com)
#    11/14 Miroslav Vadkerti (mvadkert@redhat.com)
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2
disable_ssh_strong_rng

append_cleanup "faillock --user root --reset"

# try good and bad password for root login - should fail
for PASS in $PASSWD badpassword; do
    AUDITMARK=$(get_audit_mark)

    # try to connect to root
    ssh_connect_pass $TEST_USER $TEST_USER_PASSWD root $PASS
    echo "RET=$?"

    MSG="op=PAM:authentication grantors=\? acct=\"root\""
    MSG="$MSG exe=\"/usr/sbin/sshd\" hostname=localhost addr=::1 terminal=ssh res=failed"
    augrok --seek $AUDITMARK type=USER_AUTH
    augrok --seek $AUDITMARK type=USER_AUTH msg_1=~"$MSG" || \
        exit_fail "Failed authentication attempt for user $TUSR not audited correctly"

    # make sure root is not locked out via faillock
    faillock --user root --reset
done

exit_pass
