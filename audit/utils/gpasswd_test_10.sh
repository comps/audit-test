#!/bin/sh
# \
exec expect "$0" ${1+"$@"}
######################################################################
##   Copyright (C) International Business Machines  Corp., 2003
##
##   This program is free software;  you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published by
##   the Free Software Foundation; either version 2 of the License, or
##   (at your option) any later version.
##
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY;  without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
##   the GNU General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with this program;  if not, write to the Free Software
##   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##
##
##
##  FILE   : gpasswd_test_10.sh
##
##  HISTORY:
##    07/03 Originated by Michael A. Halcrow <mike@halcrow.us>
##
######################################################################

set timeout 15

spawn /usr/bin/gpasswd $argv

expect timeout {
    send_user "Program not responding\n"
    exit -1
} "New Password:" {
    send "42\r"
}

expect timeout {
    send_user "Program not responding\n"
    exit -1
} "Re-enter new password:" {
    send_user "Changing password...\n"
    send "42\r"
    exp_continue
}
