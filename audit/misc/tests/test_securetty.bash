#!/bin/bash
###############################################################################
#   Copyright 2011 atsec information security corp.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
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
# To verify securetty

source misc_functions.bash || exit 2

# turn off screen in /etc/profile
backup /etc/profile
sed -i -e 's/\[ -w .*\]/false/' /etc/profile

# calling login in this manner leaves an entry in /var/run/utmp
# use backup (and automatic restore) to work around this
backup /var/run/utmp

# We will be modifying /etc/securetty, so back we it up.
backup /etc/securetty

# $1: full tty name
# $2: user name
# $3: password
testlogin() {
	err=0
	# username
	./do_tty $1 $2\\n
	sleep 3
	# password
	./do_tty $1 $3\\n
	sleep 3
	# newline (mls may ask a question at login)
	./do_tty $1 \\n
	sleep 2

	ttyname=$(basename $1)
	ret=$(ps -efa | grep $ttyname | grep $2 | grep -q bash)
	if [ "$?" -ne "0" ]; then
		let err=$err+1
	fi

	./do_tty $1 exit\\n
	sleep 2

	return $err
}


#  Test case: Verification of the correct operation of /etc/securetty.
#             1. Verify user can log in if tty is in /etc/securetty.
#             1. Verify user can not log in if tty is not in /etc/securetty.

(
	# find a usable tty
	TTY=$(ps ax | grep tty[0-9].*getty | grep -o tty[0-9] | head -1)
	if [ "$TTY" = "" ]; then
		exit_error "No suitable TTY found for test"
	fi

	# Add $TTY to /etc/securetty
	grep -q "^$TTY$" /etc/securetty || echo "$TTY" >> /etc/securetty

	# Verify that root login succeeds
	testlogin /dev/$TTY root $PASSWD
	if [ "$?" -ne "0" ]; then
		echo "ERROR - login attempt failed when it should have succeeded."
		exit_fail
	fi

	# Remove $TTY from /etc/securetty
	grep -v "^$TTY$" /etc/securetty > /etc/securetty.new
	mv -f /etc/securetty.new /etc/securetty

	# Verify that root login fails
	testlogin /dev/$TTY  root $PASSWD
	if [ "$?" -eq "0" ]; then
		echo "ERROR - login attempt succeeded when it should have failed."
		exit_fail
	fi

	exit_pass
)
