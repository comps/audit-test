#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
# FIXME

source tp_functions.bash || exit 2

# setup
useradd -n -u $uid $user || exit_error "useradd failed"
password=$(perl -le 'print crypt "drowssap", "42"') || exit_error "perl failed"

# test
setpid usermod -p "$password" $user || exit_error "usermod failed"

for msg_1 in \
    "usermod: op=changing password acct=$user res=success"
do
    augrep -q type=USER_CHAUTHTOK \
            user_pid=$pid \
            uid=$EUID \
            auid=$(</proc/self/loginuid) \
            msg_1="$msg_1" || exit_fail "missing: \"$msg_1\""
done

exit_pass
